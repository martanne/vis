#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lzma.h>
#include <libtar.h>

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

lzma_stream strm = LZMA_STREAM_INIT;

static int libtar_xzopen(void* call_data, const char *pathname,
		int oflags, mode_t mode) {
	int ret = 0;

	if ((ret = lzma_stream_decoder (&strm, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED)) != LZMA_OK) {
		fprintf (stderr, "lzma_stream_decoder error: %d\n", (int) ret);
		goto out;
	}

	strm.next_in = vis_single_payload;
	strm.avail_in = sizeof(vis_single_payload);

out:
	return (int)ret;
}

static int libtar_xzclose(void* call_data) {
	lzma_end(&strm);

	return 0;
}

static ssize_t libtar_xzread(void* call_data, void* buf, size_t count) {
	lzma_ret ret_xz;
	int ret = count;

	strm.next_out = buf;
	strm.avail_out = count;

	ret_xz = lzma_code(&strm, LZMA_FINISH);

	if ((ret_xz != LZMA_OK) && (ret_xz != LZMA_STREAM_END)) {
		fprintf (stderr, "lzma_code error: %d\n", (int)ret);
		ret = -1;
		goto out;
	}

	if (ret_xz == LZMA_STREAM_END)
		ret = count - strm.avail_out;

out:
	return ret;
}

static ssize_t libtar_xzwrite(void* call_data, const void* buf, size_t count) {
	return 0;
}

tartype_t xztype = {
	(openfunc_t) libtar_xzopen,
	(closefunc_t) libtar_xzclose,
	(readfunc_t) libtar_xzread,
	(writefunc_t) libtar_xzwrite
};

int extract(char *directory) {
	TAR * tar;

	if (tar_open(&tar, NULL, &xztype, O_RDONLY, 0, 0) == -1) {
		fprintf(stderr, "tar_open(): %s\n", strerror(errno));
		return -1;
	}

	if (tar_extract_all(tar, directory) != 0) {
		fprintf(stderr, "tar_extract_all(): %s\n", strerror(errno));
		return -1;
	}

	if (tar_close(tar) != 0) {
		fprintf(stderr, "tar_close(): %s\n", strerror(errno));
		return -1;
	}

	return 0;
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
	char *tmp_dirname = mkdtemp(tmp_dirname_template);

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

	if (extract(tmp_dirname) != 0)
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
