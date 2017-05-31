#include <sys/wait.h>
#include <ftw.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <archive_entry.h>
#include <archive.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "vis-single-payload.inc"

#ifndef VIS_TMP
#define VIS_TMP "/tmp/.vis-single-XXXXXX"
#endif

#ifndef VIS_TERMINFO
#define VIS_TERMINFO "/etc/terminfo:/lib/terminfo:/usr/share/terminfo:" \
	"/usr/lib/terminfo:/usr/local/share/terminfo:/usr/local/lib/terminfo"
#endif

static int copy_data(struct archive *ar, struct archive *aw) {
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

static int extract(void) {
	struct archive *in = NULL, *out = NULL;
	struct archive_entry *entry;
	int r = ARCHIVE_FAILED;

	if (!(in = archive_read_new()))
		return ARCHIVE_FAILED;

	if ((r = archive_read_support_filter_xz(in)) != ARCHIVE_OK)
		goto err;

	if ((r = archive_read_support_format_tar(in)) != ARCHIVE_OK)
		goto err;

	if (!(out = archive_write_disk_new())) {
		r = ARCHIVE_FAILED;
		goto err;
	}

	if ((r = archive_read_open_memory(in, vis_single_payload, sizeof(vis_single_payload))) != ARCHIVE_OK) {
		fprintf(stderr, "archive_read_open_memory() failed: %s\n", archive_error_string(in));
		goto err;
	}

	while (archive_read_next_header(in, &entry) == ARCHIVE_OK) {
		if ((r = archive_write_header(out, entry)) != ARCHIVE_OK) {
			fprintf(stderr, "archive_write_header() failed: %s\n", archive_error_string(out));
			goto err;
		}

		if ((r = copy_data(in, out)) != ARCHIVE_OK)
			goto err;

		if ((r = archive_write_finish_entry(out)) != ARCHIVE_OK) {
			fprintf(stderr, "archive_write_finish_entry() failed: %s\n", archive_error_string(out));
			goto err;
		 }
	}

err:
	if (out) {
		archive_write_close(out);
		archive_write_free(out);
	}

	if (in) {
		archive_read_close(in);
		archive_read_free(in);
	}

	return r;
}

static int unlink_cb(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	return remove(path);
}

int main(int argc, char **argv) {
	char exe[256], cwd[PATH_MAX], path[PATH_MAX];
	int rc = EXIT_FAILURE;

	if (!getcwd(cwd, sizeof(cwd))) {
		perror("getcwd");
		return rc;
	}

	char tmp_dirname_template[] = VIS_TMP;
	const char *tmp_dirname = mkdtemp(tmp_dirname_template);

	if (!tmp_dirname) {
		perror("mkdtemp");
		return rc;
	}

	char *old_path = getenv("PATH");
	if (snprintf(path, sizeof(path), "%s%s%s", tmp_dirname,
	             old_path ? ":" : "", old_path ? old_path : "") < 0) {
		goto err;
	}

	if (setenv("PATH", path, 1) == -1 ||
	    setenv("TERMINFO_DIRS", VIS_TERMINFO, 0) == -1) {
		perror("setenv");
		goto err;
	}


	if (chdir(tmp_dirname) == -1) {
		perror("chdir");
		goto err;
	}

	if (extract() != ARCHIVE_OK)
		goto err;

	if (chdir(cwd) == -1) {
		perror("chdir");
		goto err;
	}

	if (snprintf(exe, sizeof(exe), "%s/vis", tmp_dirname) < 0)
		goto err;

	int child_pid = fork();
	if (child_pid == -1) {
		perror("fork");
		goto err;
	} else if (child_pid == 0) {
		execv(exe, argv);
		perror("execv");
		return EXIT_FAILURE;
	}

	for (;;) {
		int status;
		int w = waitpid(child_pid, &status, 0);
		if (w == -1) {
			perror("waitpid");
			break;
		}
		if (w == child_pid) {
			rc = WEXITSTATUS(status);
			break;
		}
	}

err:
	nftw(tmp_dirname, unlink_cb, 64, FTW_DEPTH|FTW_PHYS|FTW_MOUNT);
	return rc;
}
