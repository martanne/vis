#ifndef VIS_SUBPROCESS_H
#define VIS_SUBPROCESS_H
#include "vis-core.h"
#include <sys/select.h>

typedef struct Process Process;

struct Process {
	char *name;
	int outfd;
	int errfd;
	int inpfd;
	pid_t pid;
	void **invalidator;
	Process *next;
};

typedef enum { STDOUT, STDERR, SIGNAL, EXIT } ResponseType;

Process *vis_process_communicate(Vis *, const char *command, const char *name,
                                 void **invalidator);
int vis_process_before_tick(fd_set *);
void vis_process_tick(Vis *, fd_set *);
#endif
