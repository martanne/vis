#define _GNU_SOURCE

#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <archive.h>
#include <archive_entry.h>
#include <sys/acl.h>

#include "vis-single-payload.inc"

int copy_data(struct archive *ar, struct archive *aw) {
	int r;
	const void *buff;
	size_t size;
	int64_t offset;

	for (;;) {
		if ((r = archive_read_data_block(ar, &buff, &size, &offset)) == ARCHIVE_EOF)
			return ARCHIVE_OK;

		if (r != ARCHIVE_OK) {
			fprintf(stderr, "archive_read_data_block() failed: %s\n", archive_error_string(ar));
			return r;
		}

		if ((r = archive_write_data_block(aw, buff, size, offset)) != ARCHIVE_OK) {
			fprintf(stderr, "archive_write_data_block() failed: %s\n", archive_error_string(aw));
			return r;
		}
	}

	return ARCHIVE_OK;
}

int extract(const char *path) {
	struct archive *a;
	struct archive_entry *entry;
	struct archive *ext;
	char * abs_path = NULL;
	const char * term;
	char * termenv;
	int r = ARCHIVE_FAILED;

	if ((a = archive_read_new()) == NULL)
		return ARCHIVE_FAILED;

	if ((r = archive_read_support_filter_xz(a)) != ARCHIVE_OK)
		goto out10;

	if ((r = archive_read_support_format_tar(a)) != ARCHIVE_OK)
		goto out10;

	if ((ext = archive_write_disk_new()) == NULL) {
		r = ARCHIVE_FAILED;
		goto out10;
	}

	if ((r = archive_read_open_memory(a, vis_single_payload, sizeof(vis_single_payload))) != ARCHIVE_OK) {
		fprintf(stderr, "archive_read_open_memory() failed: %s\n", archive_error_string(a));
		goto out20;
	}

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		if (asprintf(&abs_path, "%s/%s", path, archive_entry_pathname(entry)) == -1)
			goto out20;

		archive_entry_set_pathname(entry, abs_path);
		free(abs_path);

		if ((r = archive_write_header(ext, entry)) != ARCHIVE_OK) {
			fprintf(stderr, "archive_write_header() failed: %s\n", archive_error_string(ext));
			goto out20;
		}

		if ((r = copy_data(a, ext)) != ARCHIVE_OK)
			goto out20;

		if ((r = archive_write_finish_entry(ext)) != ARCHIVE_OK) {
			fprintf(stderr, "archive_write_finish_entry() failed: %s\n", archive_error_string(ext));
			goto out20;
		 }
	}

out20:
	archive_write_close(ext);
	archive_write_free(ext);

out10:
	archive_read_close(a);
	archive_read_free(a);

	return ARCHIVE_OK;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	return remove(fpath);
}

int main(int argc, char **argv) {
	char tmp_dirname_template[] = "/tmp/.vis-single-XXXXXX";
	const char *tmp_dirname;
	char *abs_path = NULL;
	const char *term;
	char *termenv = NULL;
	int child_pid, statval, rc = EXIT_FAILURE;

	if ((tmp_dirname = mkdtemp(tmp_dirname_template)) == NULL) {
		perror ("mkdtemp: Could not create tmp directory");
		goto out10;
	}

	if (extract(tmp_dirname) != ARCHIVE_OK)
		goto out20;

	if (asprintf(&abs_path, "%s/vis", tmp_dirname) == -1)
		goto out20;

	child_pid = fork();
	if (child_pid == -1) {
		fprintf(stderr, "could not fork!\n");
		goto out30;
	} else if (child_pid == 0) {
		if ((term = getenv("TERM")) == NULL) {
			puts("TERM is not defined!");
			goto out30;
		}

		if (asprintf(&termenv, "TERM=%s", term) == -1)
			goto out30;

		char *env[] = { termenv,  NULL };
		execve(abs_path, argv, env);
	}

	waitpid(child_pid, &statval, WUNTRACED|WCONTINUED);
	rc = WEXITSTATUS(statval);

out30:
	free(abs_path);
	free(termenv);

out20:
	nftw(tmp_dirname, unlink_cb, 64, FTW_DEPTH|FTW_PHYS);

out10:
	return rc;
}
