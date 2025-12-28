/* Allocate blocks holding the actual file content in chunks of size: */
#ifndef BLOCK_SIZE
#define BLOCK_SIZE (1 << 20)
#endif
/* Files smaller than this value are copied on load, larger ones are mmap(2)-ed
 * directly. Hence the former can be truncated, while doing so on the latter
 * results in havoc. */
#define BLOCK_MMAP_SIZE (1 << 26)

/* allocate a new block of MAX(size, BLOCK_SIZE) bytes */
static Block *block_alloc(size_t size)
{
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

static void block_free(Block *blk)
{
	if (!blk)
		return;
	if (blk->type == BLOCK_TYPE_MALLOC)
		free(blk->data);
	else if ((blk->type == BLOCK_TYPE_MMAP_ORIG || blk->type == BLOCK_TYPE_MMAP) && blk->data)
		munmap(blk->data, blk->size);
	free(blk);
}

static Block *block_read(size_t size, int fd)
{
	Block *blk = block_alloc(size);
	if (!blk)
		return NULL;
	char *data = blk->data;
	size_t rem = size;
	while (rem > 0) {
		ssize_t len = read(fd, data, rem);
		if (len == -1 && errno != EINTR) {
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

static Block *block_mmap(size_t size, int fd, off_t offset)
{
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

static Block *block_load(int dirfd, const char *filename, enum TextLoadMethod method, struct stat *info)
{
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

/* check whether block has enough free space to store len bytes */
static bool block_capacity(Block *blk, size_t len)
{
	return blk->size - blk->len >= len;
}

/* append data to block, assumes there is enough space available */
static const char *block_append(Block *blk, const char *data, size_t len)
{
	char *dest = memcpy(blk->data + blk->len, data, len);
	blk->len += len;
	return dest;
}

/* insert data into block at an arbitrary position, this should only be used with
 * data of the most recently created piece. */
static bool block_insert(Block *blk, size_t pos, const char *data, size_t len)
{
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
static bool block_delete(Block *blk, size_t pos, size_t len)
{
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

static Block *text_block_mmaped(Text *txt)
{
	Block *result = 0;
	if (txt->count > 0 && txt->data[0]->type == BLOCK_TYPE_MMAP_ORIG && txt->data[0]->size)
		result = txt->data[0];
	return result;
}

/* preserve the current text content such that it can be restored by
 * means of undo/redo operations */
void text_snapshot(Text *txt)
{
	if (txt->current_revision)
		txt->last_revision = txt->current_revision;
	txt->current_revision = NULL;
	txt->cache = NULL;
}

static void text_saved(Text *txt, struct stat *meta)
{
	if (meta)
		txt->info = *meta;
	txt->saved_revision = txt->history;
	text_snapshot(txt);
}

Text *text_load(Vis *vis, const char *filename)
{
	return text_load_method(vis, filename, TEXT_LOAD_AUTO);
}

Text *text_load_method(Vis *vis, const char *filename, enum TextLoadMethod method)
{
	return text_loadat_method(vis, AT_FDCWD, filename, method);
}

ssize_t write_all(int fd, const char *buf, size_t count) {
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
		int ret = fchdir(cwd);
		close(cwd);
		if (ret != 0)
		  return -1;
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
static bool text_save_begin_atomic(TextSave *ctx)
{
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

	str8 base, dir, fname = str8_from_c_str((char *)ctx->filename);
	path_split(fname, &dir, &base);

	char suffix[] = ".vis.XXXXXX";
	size_t len = fname.length + sizeof("./.") + sizeof(suffix);

	if (!(ctx->tmpname.data = malloc(len)))
		goto err;

	ctx->tmpname.length = snprintf((char *)ctx->tmpname.data, len, "%.*s/.%.*s%s",
	                               (int)dir.length, dir.data, (int)base.length, base.data, suffix);

	if ((ctx->fd = mkstempat(ctx->dirfd, (char *)ctx->tmpname.data)) == -1)
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

	return true;
err:
	saved_errno = errno;
	if (oldfd != -1)
		close(oldfd);
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

	if (renameat(ctx->dirfd, (char *)ctx->tmpname.data, ctx->dirfd, ctx->filename) == -1)
		return false;


	str8 directory;
	path_split(ctx->tmpname, &directory, 0);
	/* NOTE: tmpname was allocated by us, no issue with writing a 0 into it;
	 * however, we may have gotten a static "." back so we shouldn't write in that case. */
	if (directory.data[directory.length] != 0) directory.data[directory.length] = 0;

	int dir = openat(ctx->dirfd, (char *)directory.data, O_DIRECTORY|O_RDONLY);

	free(ctx->tmpname.data);
	ctx->tmpname.data = 0;

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
	return true;
err:
	saved_errno = errno;
	if (newfd != -1)
		close(newfd);
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

bool text_save_begin(TextSave *ctx) {
	enum TextSaveMethod type = ctx->method;
	errno = 0;
	if ((type == TEXT_SAVE_AUTO || type == TEXT_SAVE_ATOMIC) && text_save_begin_atomic(ctx)) {
		ctx->method = TEXT_SAVE_ATOMIC;
		return true;
	}
	if (errno == ENOSPC)
		goto err;
	if ((type == TEXT_SAVE_AUTO || type == TEXT_SAVE_INPLACE) && text_save_begin_inplace(ctx)) {
		ctx->method = TEXT_SAVE_INPLACE;
		return true;
	}
err:
	text_save_cancel(ctx);
	return false;
}

void text_save_cancel(TextSave *ctx) {
	int saved_errno = errno;
	if (ctx->fd != -1)
		close(ctx->fd);
	if (ctx->tmpname.data && ctx->tmpname.data[0])
		unlinkat(ctx->dirfd, (char *)ctx->tmpname.data, 0);
	free(ctx->tmpname.data);
	errno = saved_errno;
}

bool text_save_commit(TextSave *ctx) {
	bool result = false;
	switch (ctx->method) {
	case TEXT_SAVE_ATOMIC:  result = text_save_commit_atomic(ctx);  break;
	case TEXT_SAVE_INPLACE: result = text_save_commit_inplace(ctx); break;
	default: break;
	}
	text_save_cancel(ctx);
	return result;
}

void text_mark_current_revision(Text *txt) { text_saved(txt, 0); }

ssize_t text_save_write_range(TextSave *ctx, const Filerange *range) {
	return text_write_range(ctx->txt, range, ctx->fd);
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
