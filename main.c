#include <signal.h>
#include <string.h>
#include <errno.h>

#include "ui-curses.h"
#include "vis.h"

static Vis *vis;

static void signal_handler(int signum, siginfo_t *siginfo, void *context) {
	vis_signal_handler(vis, signum, siginfo, context);
}

int main(int argc, char *argv[]) {

	vis = vis_new(ui_curses_new());

	/* install signal handlers etc. */
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = signal_handler;
	if (sigaction(SIGBUS, &sa, NULL) || sigaction(SIGINT, &sa, NULL))
		vis_die(vis, "sigaction: %s", strerror(errno));

	sigset_t blockset;
	sigemptyset(&blockset);
	sigaddset(&blockset, SIGWINCH);
	sigprocmask(SIG_BLOCK, &blockset, NULL);
	signal(SIGPIPE, SIG_IGN);

	vis_run(vis, argc, argv);
	vis_free(vis);
	return 0;
}
