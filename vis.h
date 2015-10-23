#ifndef VIS_H
#define VIS_H

#include "ui.h"
#include "editor.h"

typedef Editor Vis;

Vis *vis_new(Ui*);
#define vis_free editor_free

void vis_run(Vis*, int argc, char *argv[]);
void vis_die(Vis*, const char *msg, ...);

enum VisMode {
	VIS_MODE_BASIC,
	VIS_MODE_MOVE,
	VIS_MODE_OPERATOR,
	VIS_MODE_OPERATOR_OPTION,
	VIS_MODE_NORMAL,
	VIS_MODE_TEXTOBJ,
	VIS_MODE_VISUAL,
	VIS_MODE_VISUAL_LINE,
	VIS_MODE_READLINE,
	VIS_MODE_PROMPT,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE,
	VIS_MODE_LAST,
};

enum VisOperator {
	OP_DELETE,
	OP_CHANGE,
	OP_YANK,
	OP_PUT,
	OP_SHIFT_RIGHT,
	OP_SHIFT_LEFT,
	OP_CASE_CHANGE,
	OP_JOIN,
	OP_REPEAT_INSERT,
	OP_REPEAT_REPLACE,
	OP_CURSOR,
};

bool vis_signal_handler(Vis*, int signum, const siginfo_t *siginfo,
	const void *context);

#endif
