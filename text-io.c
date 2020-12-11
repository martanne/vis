#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#if CONFIG_ACL
#include <sys/acl.h>
#endif
#if CONFIG_SELINUX
#include <selinux/selinux.h>
#endif

#include "text.h"
#include "text-internal.h"
#include "text-util.h"
#include "util.h"

struct TextSave {                  /* used to hold context between text_save_{begin,commit} calls */
	Text *txt;                 /* text to operate on */
	char *filename;            /* filename to save to as given to text_save_begin */
	char *tmpname;             /* temporary name used for atomic rename(2) */
	int fd;                    /* file descriptor to write data to using text_save_write */
	int dirfd;                 /* directory file descriptor, relative to which we save */
	enum TextSaveMethod type;  /* method used to save file */
};

/* Allocate blocks holding the actual file content in chunks of size: */
#ifndef BLOCK_SIZE
#define BLOCK_SIZE (1 << 20)
#endif
/* Files smaller than this value are copied on load, larger ones are mmap(2)-ed
 * directly. Hence the former can be truncated, while doing so on the latter
 * results in havoc. */
#define BLOCK_MMAP_SIZE (1 << 26)

/* allocate a new block of MAX(size, BLOCK_SIZE) bytes */
Block *block_alloc(size_t size) {
	Block *blk = calloc(1, sizeof *blk);
	if (!blk)
		return NULL;
	if (BLOCK_SIZE > size)
		size = BLOCK_SIZE;
	if (!(blk->data = malloc(size))) {
		free(blk);
		return NULL;
	}
	blk->type = BLOCK_TYPE_MALLOC;
	blk->size = size;
	return blk;
}

Block *block_read(size_t size, int fd) {
	Block *blk = block_alloc(size);
	if (!blk)
		return NULL;
	char *data = blk->data;
	size_t rem = size;
	while (rem > 0) {
		ssize_t len = read(fd, data, rem);
		if (len == -1) {
			block_free(blk);
			return NULL;
		} else if (len == 0) {
			break;
		} else {
			data += len;
			rem -= len;
		}
	}
	blk->len = size - rem;
	return blk;
}

Block *block_mmap(size_t size, int fd, off_t offset) {
	Block *blk = calloc(1, sizeof *blk);
	if (!blk)
		return NULL;
	if (size) {
		blk->data = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, offset);
		if (blk->data == MAP_FAILED) {
			free(blk);
			return NULL;
		}
	}
	blk->type = BLOCK_TYPE_MMAP_ORIG;
	blk->size = size;
	blk->len = size;
	return blk;
}

Block *block_load(int dirfd, const char *filename, enum TextLoadMethod method, struct stat *info) {
	Block *block = NULL;
	int fd = openat(dirfd, filename, O_RDONLY);
	if (fd == -1)
		goto out;
	if (fstat(fd, info) == -1)
		goto out;
	if (!S_ISREG(info->st_mode)) {
		errno = S_ISDIR(info->st_mode) ? EISDIR : ENOTSUP;
		goto out;
	}

	// XXX: use lseek(fd, 0, SEEK_END); instead?
	size_t size = info->st_size;
	if (size == 0)
		goto out;
	if (method == TEXT_LOAD_READ || (method == TEXT_LOAD_AUTO && size < BLOCK_MMAP_SIZE))
		block = block_read(size, fd);
	else
		block = block_mmap(size, fd, 0);
out:
	if (fd != -1)
		close(fd);
	return block;
}

void block_free(Block *blk) {
	if (!blk)
		return;
	if (blk->type == BLOCK_TYPE_MALLOC)
		free(blk->data);
	else if ((blk->type == BLOCK_TYPE_MMAP_ORIG || blk->type == BLOCK_TYPE_MMAP) && blk->data)
		munmap(blk->data, blk->size);
	free(blk);
}

/* check whether block has enough free space to store len bytes */
bool block_capacity(Block *blk, size_t len) {
	return blk->size - blk->len >= len;
}

/* append data to block, assumes there is enough space available */
const char *block_append(Block *blk, const char *data, size_t len) {
	char *dest = memcpy(blk->data + blk->len, data, len);
	blk->len += len;
	return dest;
}

/* insert data into block at an arbitrary position, this should only be used with
 * data of the most recently created piece. */
bool block_insert(Block *blk, size_t pos, const char *data, size_t len) {
	if (pos > blk->len || !block_capacity(blk, len))
		return false;
	if (blk->len == pos)
		return block_append(blk, data, len);
	char *insert = blk->data + pos;
	memmove(insert + len, insert, blk->len - pos);
	memcpy(insert, data, len);
	blk->len += len;
	return true;
}

/* delete data from a block at an arbitrary position, this should only be used with
 * data of the most recently created piece. */
bool block_delete(Block *blk, size_t pos, size_t len) {
	size_t end;
	if (!addu(pos, len, &end) || end > blk->len)
		return false;
	if (blk->len == pos) {
		blk->len -= len;
		return true;
	}
	char *delete = blk->data + pos;
	memmove(delete, delete + len, blk->len - pos - len);
	blk->len -= len;
	return true;
}

Text *text_load(const char *filename) {
	return text_load_method(filename, TEXT_LOAD_AUTO);
}

Text *text_loadat(int dirfd, const char *filename) {
	return text_loadat_method(dirfd, filename, TEXT_LOAD_AUTO);
}

Text *text_load_method(const char *filename, enum TextLoadMethod method) {
	return text_loadat_method(AT_FDCWD, filename, method);
}

static ssize_t write_all(int fd, const char *buf, size_t count) {
	size_t rem = count;
	while (rem > 0) {
		ssize_t written = write(fd, buf, rem > INT_MAX ? INT_MAX : rem);
		if (written < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return -1;
		} else if (written == 0) {
			break;
		}
		rem -= written;
		buf += written;
	}
	return count - rem;
}

static bool preserve_acl(int src, int dest) {
#if CONFIG_ACL
	acl_t acl = acl_get_fd(src);
	if (!acl)
		return errno == ENOTSUP ? true : false;
	if (acl_set_fd(dest, acl) == -1) {
		acl_free(acl);
		return false;
	}
	acl_free(acl);
#endif /* CONFIG_ACL */
	return true;
}

static bool preserve_selinux_context(int src, int dest) {
#if CONFIG_SELINUX
	char *context = NULL;
	if (!is_selinux_enabled())
		return true;
	if (fgetfilecon(src, &context) == -1)
		return errno == ENOTSUP ? true : false;
	if (fsetfilecon(dest, context) == -1) {
		freecon(context);
		return false;
	}
	freecon(context);
#endif /* CONFIG_SELINUX */
	return true;
}

static int mkstempat(int dirfd, char *template) {
	if (dirfd == AT_FDCWD)
		return mkstemp(template);
	// FIXME: not thread safe
	int fd = -1;
	int cwd = open(".", O_RDONLY|O_DIRECTORY);
	if (cwd == -1)
		goto err;
	if (fchdir(dirfd) == -1)
		goto err;
	fd = mkstemp(template);
err:
	if (cwd != -1) {
		fchdir(cwd);
		close(cwd);
	}
	return fd;
}

/* Create a new file named `.filename.vis.XXXXXX` (where `XXXXXX` is a
 * randomly generated, unique suffix) and try to preserve all important
 * meta data. After the file content has been written to this temporary
 * file, text_save_commit_atomic will atomically move it to  its final
 * (possibly already existing) destination using rename(2).
 *
 * This approach does not work if:
 *
 *   - the file is a symbolic link
 *   - the file is a hard link
 *   - file ownership can not be preserved
 *   - file group can not be preserved
 *   - directory permissions do not allow creation of a new file
 *   - POSIX ACL can not be preserved (if enabled)
 *   - SELinux security context can not be preserved (if enabled)
 */
static bool text_save_begin_atomic(TextSave *ctx) {
	int oldfd, saved_errno;
	if ((oldfd = openat(ctx->dirfd, ctx->filename, O_RDONLY)) == -1 && errno != ENOENT)
		goto err;
	struct stat oldmeta = { 0 };
	if (oldfd != -1 && fstatat(ctx->dirfd, ctx->filename, &oldmeta, AT_SYMLINK_NOFOLLOW) == -1)
		goto err;
	if (oldfd != -1) {
		if (S_ISLNK(oldmeta.st_mode)) /* symbolic link */
			goto err;
		if (oldmeta.st_nlink > 1) /* hard link */
			goto err;
	}

	char suffix[] = ".vis.XXXXXX";
	size_t len = strlen(ctx->filename) + sizeof("./.") + sizeof(suffix);
	char *dir = strdup(ctx->filename);
	char *base = strdup(ctx->filename);

	if (!(ctx->tmpname = malloc(len)) || !dir || !base) {
		free(dir);
		free(base);
		goto err;
	}

	snprintf(ctx->tmpname, len, "%s/.%s%s", dirname(dir), basename(base), suffix);
	free(dir);
	free(base);

	if ((ctx->fd = mkstempat(ctx->dirfd, ctx->tmpname)) == -1)
		goto err;

	if (oldfd == -1) {
		mode_t mask = umask(0);
		umask(mask);
		if (fchmod(ctx->fd, 0666 & ~mask) == -1)
			goto err;
	} else {
		if (fchmod(ctx->fd, oldmeta.st_mode) == -1)
			goto err;
		if (!preserve_acl(oldfd, ctx->fd) || !preserve_selinux_context(oldfd, ctx->fd))
			goto err;
		/* change owner if necessary */
		if (oldmeta.st_uid != getuid() && fchown(ctx->fd, oldmeta.st_uid, (uid_t)-1) == -1)
			goto err;
		/* change group if necessary, in case of failure some editors reset
		 * the group permissions to the same as for others */
		if (oldmeta.st_gid != getgid() && fchown(ctx->fd, (uid_t)-1, oldmeta.st_gid) == -1)
			goto err;
		close(oldfd);
	}

	ctx->type = TEXT_SAVE_ATOMIC;
	return true;
err:
	saved_errno = errno;
	if (oldfd != -1)
		close(oldfd);
	if (ctx->fd != -1)
		close(ctx->fd);
	ctx->fd = -1;
	free(ctx->tmpname);
	ctx->tmpname = NULL;
	errno = saved_errno;
	return false;
}

static bool text_save_commit_atomic(TextSave *ctx) {
	if (fsync(ctx->fd) == -1)
		return false;

	struct stat meta = { 0 };
	if (fstat(ctx->fd, &meta) == -1)
		return false;

	bool close_failed = (close(ctx->fd) == -1);
	ctx->fd = -1;
	if (close_failed)
		return false;

	if (renameat(ctx->dirfd, ctx->tmpname, ctx->dirfd, ctx->filename) == -1)
		return false;

	free(ctx->tmpname);
	ctx->tmpname = NULL;

	int dir = openat(ctx->dirfd, dirname(ctx->filename), O_DIRECTORY|O_RDONLY);
	if (dir == -1)
		return false;

	if (fsync(dir) == -1 && errno != EINVAL) {
		close(dir);
		return false;
	}

	if (close(dir) == -1)
		return false;

	text_saved(ctx->txt, &meta);
	return true;
}

static bool text_save_begin_inplace(TextSave *ctx) {
	Text *txt = ctx->txt;
	struct stat now = { 0 };
	int newfd = -1, saved_errno;
	if ((ctx->fd = openat(ctx->dirfd, ctx->filename, O_CREAT|O_WRONLY, 0666)) == -1)
		goto err;
	if (fstat(ctx->fd, &now) == -1)
		goto err;
	struct stat loaded = text_stat(txt);
	Block *block = text_block_mmaped(txt);
	if (block && now.st_dev == loaded.st_dev && now.st_ino == loaded.st_ino) {
		/* The file we are going to overwrite is currently mmap-ed from
		 * text_load, therefore we copy the mmap-ed block to a temporary
		 * file and remap it at the same position such that all pointers
		 * from the various pieces are still valid.
		 */
		size_t size = block->size;
		char tmpname[32] = "/tmp/vis-XXXXXX";
		newfd = mkstemp(tmpname);
		if (newfd == -1)
			goto err;
		if (unlink(tmpname) == -1)
			goto err;
		ssize_t written = write_all(newfd, block->data, size);
		if (written == -1 || (size_t)written != size)
			goto err;
		void *data = mmap(block->data, size, PROT_READ, MAP_SHARED|MAP_FIXED, newfd, 0);
		if (data == MAP_FAILED)
			goto err;
		bool close_failed = (close(newfd) == -1);
		newfd = -1;
		if (close_failed)
			goto err;
		block->type = BLOCK_TYPE_MMAP;
	}
	/* overwrite the existing file content, if something goes wrong
	 * here we are screwed, TODO: make a backup before? */
	if (ftruncate(ctx->fd, 0) == -1)
		goto err;
	ctx->type = TEXT_SAVE_INPLACE;
	return true;
err:
	saved_errno = errno;
	if (newfd != -1)
		close(newfd);
	if (ctx->fd != -1)
		close(ctx->fd);
	ctx->fd = -1;
	errno = saved_errno;
	return false;
}

static bool text_save_commit_inplace(TextSave *ctx) {
	if (fsync(ctx->fd) == -1)
		return false;
	struct stat meta = { 0 };
	if (fstat(ctx->fd, &meta) == -1)
		return false;
	if (close(ctx->fd) == -1)
		return false;
	text_saved(ctx->txt, &meta);
	return true;
}

TextSave *text_save_begin(Text *txt, int dirfd, const char *filename, enum TextSaveMethod type) {
	if (!filename)
		return NULL;
	TextSave *ctx = calloc(1, sizeof *ctx);
	if (!ctx)
		return NULL;
	ctx->txt = txt;
	ctx->fd = -1;
	ctx->dirfd = dirfd;
	if (!(ctx->filename = strdup(filename)))
		goto err;
	errno = 0;
	if ((type == TEXT_SAVE_AUTO || type == TEXT_SAVE_ATOMIC) && text_save_begin_atomic(ctx))
		return ctx;
	if (errno == ENOSPC)
		goto err;
	if ((type == TEXT_SAVE_AUTO || type == TEXT_SAVE_INPLACE) && text_save_begin_inplace(ctx))
		return ctx;
err:
	text_save_cancel(ctx);
	return NULL;
}

bool text_save_commit(TextSave *ctx) {
	if (!ctx)
		return true;
	bool ret;
	switch (ctx->type) {
	case TEXT_SAVE_ATOMIC:
		ret = text_save_commit_atomic(ctx);
		break;
	case TEXT_SAVE_INPLACE:
		ret = text_save_commit_inplace(ctx);
		break;
	default:
		ret = false;
		break;
	}

	text_save_cancel(ctx);
	return ret;
}

void text_save_cancel(TextSave *ctx) {
	if (!ctx)
		return;
	int saved_errno = errno;
	if (ctx->fd != -1)
		close(ctx->fd);
	if (ctx->tmpname && ctx->tmpname[0])
		unlinkat(ctx->dirfd, ctx->tmpname, 0);
	free(ctx->tmpname);
	free(ctx->filename);
	free(ctx);
	errno = saved_errno;
}

/* First try to save the file atomically using rename(2) if this does not
 * work overwrite the file in place. However if something goes wrong during
 * this overwrite the original file is permanently damaged.
 */
bool text_save(Text *txt, const char *filename) {
	return text_saveat(txt, AT_FDCWD, filename);
}

bool text_saveat(Text *txt, int dirfd, const char *filename) {
	return text_saveat_method(txt, dirfd, filename, TEXT_SAVE_AUTO);
}

bool text_save_method(Text *txt, const char *filename, enum TextSaveMethod method) {
	return text_saveat_method(txt, AT_FDCWD, filename, method);
}

bool text_saveat_method(Text *txt, int dirfd, const char *filename, enum TextSaveMethod method) {
	if (!filename) {
		text_saved(txt, NULL);
		return true;
	}
	TextSave *ctx = text_save_begin(txt, dirfd, filename, method);
	if (!ctx)
		return false;
	Filerange range = (Filerange){ .start = 0, .end = text_size(txt) };
	ssize_t written = text_save_write_range(ctx, &range);
	if (written == -1 || (size_t)written != text_range_size(&range)) {
		text_save_cancel(ctx);
		return false;
	}
	return text_save_commit(ctx);
}

ssize_t text_save_write_range(TextSave *ctx, const Filerange *range) {
	return text_write_range(ctx->txt, range, ctx->fd);
}

ssize_t text_write(const Text *txt, int fd) {
	Filerange r = (Filerange){ .start = 0, .end = text_size(txt) };
	return text_write_range(txt, &r, fd);
}

ssize_t text_write_range(const Text *txt, const Filerange *range, int fd) {
	size_t size = text_range_size(range), rem = size;
	for (Iterator it = text_iterator_get(txt, range->start);
	     rem > 0 && text_iterator_valid(&it);
	     text_iterator_next(&it)) {
		size_t prem = it.end - it.text;
		if (prem > rem)
			prem = rem;
		ssize_t written = write_all(fd, it.text, prem);
		if (written == -1)
			return -1;
		rem -= written;
		if ((size_t)written != prem)
			break;
	}
	return size - rem;
}
