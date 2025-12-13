#ifndef VIS_SUBPROCESS_H
#define VIS_SUBPROCESS_H
#include "vis-core.h"
#include "vis-lua.h"

typedef struct Process Process;
#if CONFIG_LUA
typedef int Invalidator(lua_State*);
#else
typedef void Invalidator;
#endif

struct Process {
	char *name;
	int outfd;
	int errfd;
	int inpfd;
	pid_t pid;
	Invalidator** invalidator;
	Process *next;
};

typedef enum { STDOUT, STDERR, SIGNAL, EXIT } ResponseType;

VIS_INTERNAL Process *vis_process_communicate(Vis *, const char *command, const char *name,
                                              Invalidator **invalidator);
VIS_INTERNAL int vis_process_before_tick(fd_set *);
VIS_INTERNAL void vis_process_tick(Vis *, fd_set *);
VIS_INTERNAL void vis_process_waitall(Vis *);
#endif
