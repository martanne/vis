#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lzma.h>
#include <libuntar.h>

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

static lzma_stream strm = LZMA_STREAM_INIT;

static int libtar_xzopen(const char *pathname, int flags, ...) {
	int ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED);
	if (ret != LZMA_OK) {
		fprintf(stderr, "lzma_stream_decoder error: %d\n", ret);
		return ret;
	}

	strm.next_in = vis_single_payload;
	strm.avail_in = sizeof(vis_single_payload);

	return ret;
}

static int libtar_xzclose(int fd) {
	lzma_end(&strm);
	return 0;
}

static ssize_t libtar_xzread(int fd, void *buf, size_t count) {
	strm.next_out = buf;
	strm.avail_out = count;

	int ret = lzma_code(&strm, LZMA_FINISH);
	if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
		fprintf(stderr, "lzma_code error: %d\n", ret);
		return -1;
	}

	return count - strm.avail_out;
}

tartype_t xztype = {
	libtar_xzopen,
	libtar_xzclose,
	libtar_xzread,
};

int extract(char *directory) {
	TAR *tar;

	if (tar_open(&tar, NULL, &xztype, O_RDONLY, 0, 0) == -1) {
		perror("tar_open");
		return -1;
	}

	if (tar_extract_all(tar, directory) != 0) {
		perror("tar_extract_all");
		return -1;
	}

	if (tar_close(tar) != 0) {
		perror("tar_close");
		return -1;
	}

	return 0;
}

static int unlink_cb(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	return remove(path);
}

int main(int argc, char **argv) {
	int rc = EXIT_FAILURE;
	char exe[256], path[PATH_MAX];
	char tmp_dirname[] = VIS_TMP;

	if (!mkdtemp(tmp_dirname)) {
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

	if (extract(tmp_dirname) != 0)
		goto err;

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

	signal(SIGINT, SIG_IGN);

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
