#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include "vis-lua.h"
#include "vis-subprocess.h"
#include "util.h"

/* Pool of information about currently running subprocesses */
static Process *process_pool;

/**
 * Adds new empty process information structure to the process pool and
 * returns it
 * @return a new Process instance
 */
static Process *new_process_in_pool(void) {
	Process *newprocess = malloc(sizeof(Process));
	if (!newprocess) {
		return NULL;
	}
	newprocess->next = process_pool;
	process_pool = newprocess;
	return newprocess;
}

/**
 * Removes the subprocess information from the pool, sets invalidator to NULL
 * and frees resources.
 * @param a reference to a reference to the process to be removed
 * @return the next process in the pool
 */
static void destroy_process(Process **pointer) {
	Process *target = *pointer;
	if (target->outfd != -1) {
		close(target->outfd);
	}
	if (target->errfd != -1) {
		close(target->errfd);
	}
	if (target->inpfd != -1) {
		close(target->inpfd);
	}
	/* marking stream as closed for lua */
	if (target->invalidator) {
		*(target->invalidator) = NULL;
	}
	*pointer = target->next;
	free(target->name);
	free(target);
}

/**
 * Starts new subprocess by passing the `command` to the shell and
 * returns the subprocess information structure, containing file descriptors
 * of the process.
 * Also stores the subprocess information to the internal pool to track
 * its status and responses.
 * @param name a string that contains a unique name for the subprocess.
 * This name will be passed to the PROCESS_RESPONSE event handler
 * to distinguish running subprocesses.
 * @param command a command to be executed to spawn a process
 * @param invalidator a pointer to the pointer which shows that the subprocess
 * is invalid when set to NULL. When the subprocess dies, it is set to NULL.
 * If a caller sets the pointer to NULL the subprocess will be killed on the
 * next main loop iteration.
 */
Process *vis_process_communicate(Vis *vis, const char *name,
                                 const char *command, Invalidator **invalidator) {
	int pin[2], pout[2], perr[2];
	pid_t pid = (pid_t)-1;
	if (pipe(perr) == -1) {
		goto closeerr;
	}
	if (pipe(pout) == -1) {
		goto closeouterr;
	}
	if (pipe(pin) == -1) {
		goto closeall;
	}
	pid = fork();
	if (pid == -1) {
		vis_info_show(vis, "fork failed: %s", strerror(errno));
	} else if (pid == 0) { /* child process */
		sigset_t sigterm_mask;
		sigemptyset(&sigterm_mask);
		sigaddset(&sigterm_mask, SIGTERM);
		if (sigprocmask(SIG_UNBLOCK, &sigterm_mask, NULL) == -1) {
			fprintf(stderr, "failed to reset signal mask");
			exit(EXIT_FAILURE);
		}
		dup2(pin[0], STDIN_FILENO);
		dup2(pout[1], STDOUT_FILENO);
		dup2(perr[1], STDERR_FILENO);
	} else { /* main process */
		Process *new = new_process_in_pool();
		if (!new) {
			vis_info_show(vis, "Cannot create process: %s", strerror(errno));
			goto closeall;
		}
		new->name = strdup(name);
		if (!new->name) {
			vis_info_show(vis, "Cannot copy process name: %s", strerror(errno));
			/* pop top element (which is `new`) from the pool */
			destroy_process(&process_pool);
			goto closeall;
		}
		new->outfd = pout[0];
		new->errfd = perr[0];
		new->inpfd = pin[1];
		new->pid = pid;
		new->invalidator = invalidator;
		close(pin[0]);
		close(pout[1]);
		close(perr[1]);
		return new;
	}
closeall:
	close(pin[0]);
	close(pin[1]);
closeouterr:
	close(pout[0]);
	close(pout[1]);
closeerr:
	close(perr[0]);
	close(perr[1]);
	if (pid == 0) { /* start command in child process */
		execlp(vis->shell, vis->shell, "-c", command, (char*)NULL);
		fprintf(stderr, "exec failed: %s(%d)\n", strerror(errno), errno);
		exit(1);
	} else {
		vis_info_show(vis, "process creation failed: %s", strerror(errno));
	}
	return NULL;
}

/**
 * Adds file descriptors of currently running subprocesses to the `readfds`
 * to track their readiness and returns maximum file descriptor value
 * to pass it to the `pselect` call
 * @param readfds the structure for `pselect` call to fill
 * @return maximum file descriptor number in the readfds structure
 */
int vis_process_before_tick(fd_set *readfds) {
	int maxfd = 0;
	for (Process **pointer = &process_pool; *pointer; pointer = &((*pointer)->next)) {
		Process *current = *pointer;
		if (current->outfd != -1) {
			FD_SET(current->outfd, readfds);
			maxfd = maxfd < current->outfd ? current->outfd : maxfd;
		}
		if (current->errfd != -1) {
			FD_SET(current->errfd, readfds);
			maxfd = maxfd < current->errfd ? current->errfd : maxfd;
		}
	}
	return maxfd;
}

/**
 * Reads data from the given subprocess file descriptor `fd` and fires
 * the PROCESS_RESPONSE event in Lua with given subprocess `name`,
 * `rtype` and the read data as arguments.
 * @param fd the file descriptor to read data from
 * @param name a name of the subprocess
 * @param rtype a type of file descriptor where the new data is found
 */
static void read_and_fire(Vis* vis, int fd, const char *name, ResponseType rtype) {
	static char buffer[PIPE_BUF];
	size_t obtained = read(fd, &buffer, PIPE_BUF-1);
	if (obtained > 0) {
		vis_lua_process_response(vis, name, buffer, obtained, rtype);
	}
}

/**
 * Checks if `readfds` contains file descriptors of subprocesses from
 * the pool. If so, it reads their data and fires corresponding events.
 * Also checks if each subprocess from the pool is dead or needs to be
 * killed then raises an event or kills it if necessary.
 * @param readfds the structure for `pselect` call with file descriptors
 */
void vis_process_tick(Vis *vis, fd_set *readfds) {
	for (Process **pointer = &process_pool; *pointer; ) {
		Process *current = *pointer;
		if (current->outfd != -1 && FD_ISSET(current->outfd, readfds)) {
			read_and_fire(vis, current->outfd, current->name, STDOUT);
		}
		if (current->errfd != -1 && FD_ISSET(current->errfd, readfds)) {
			read_and_fire(vis, current->errfd, current->name, STDERR);
		}
		int status;
		pid_t wpid = waitpid(current->pid, &status, WNOHANG);
		if (wpid == -1)	{
			vis_message_show(vis, strerror(errno));
		} else if (wpid == current->pid) {
			goto just_destroy;
		} else if (!*(current->invalidator)) {
			goto kill_and_destroy;
		}
		pointer = &current->next;
		continue;
kill_and_destroy:
		kill(current->pid, SIGTERM);
		waitpid(current->pid, &status, 0);
just_destroy:
		if (WIFSIGNALED(status)) {
			vis_lua_process_response(vis, current->name, NULL, WTERMSIG(status), SIGNAL);
		} else {
			vis_lua_process_response(vis, current->name, NULL, WEXITSTATUS(status), EXIT);
		}
		destroy_process(pointer);
	}
}
