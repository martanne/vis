#include <stdarg.h>

#include "vis-core.h"

#if !CONFIG_LUA
bool vis_event_emit(Vis *vis, enum VisEvents id, ...) {
	if (!vis->initialized) {
		vis->initialized = true;
		ui_init(&vis->ui, vis);
	}

	va_list ap;
	va_start(ap, id);

	if (id == VIS_EVENT_WIN_STATUS) {
		Win *win = va_arg(ap, Win*);
		window_status_update(vis, win);
	}

	va_end(ap);
	return true;
}
#endif
