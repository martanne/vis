#include "vis-core.h"

static int ranges_comparator(const void *a, const void *b) {
	const Filerange *r1 = a, *r2 = b;
	if (!text_range_valid(r1))
		return text_range_valid(r2) ? 1 : 0;
	if (!text_range_valid(r2))
		return -1;
	return (r1->start < r2->start || (r1->start == r2->start && r1->end < r2->end)) ? -1 : 1;
}

void vis_mark_normalize(Array *a) {
	array_sort(a, ranges_comparator);
	Filerange *prev = NULL, *r = array_get(a, 0);
	for (size_t i = 0; r; r = array_get(a, i)) {
		if (text_range_size(r) == 0) {
			array_remove(a, i);
		} else if (prev && text_range_overlap(prev, r)) {
			*prev = text_range_union(prev, r);
			array_remove(a, i);
		} else {
			prev = r;
			i++;
		}
	}
}

bool vis_mark_equal(Array *a, Array *b) {
	if (a->len != b->len)
		return false;
	size_t len = a->len;
	for (size_t i = 0; i < len; i++) {
		if (!text_range_equal(array_get(a, i), array_get(b, i)))
			return false;
	}

	return true;
}

void mark_init(Array *arr) {
	array_init_sized(arr, sizeof(SelectionRegion));
}

void mark_release(Array *arr) {
	if (!arr)
		return;
	array_release(arr);
}

static Array *mark_from(Vis *vis, enum VisMark id) {
	if (!vis->win)
		return NULL;
	if (id == VIS_MARK_SELECTION)
		return &vis->win->saved_selections;
	File *file = vis->win->file;
	if (id < LENGTH(file->marks))
		return &file->marks[id];
	return NULL;
}

enum VisMark vis_mark_used(Vis *vis) {
	return vis->action.mark;
}

void vis_mark(Vis *vis, enum VisMark mark) {
	if (mark < LENGTH(vis->win->file->marks))
		vis->action.mark = mark;
}

static Array mark_get(Win *win, Array *mark) {
	Array sel;
	array_init_sized(&sel, sizeof(Filerange));
	if (!mark)
		return sel;
	size_t len = mark->len;
	array_reserve(&sel, len);
	for (size_t i = 0; i < len; i++) {
		SelectionRegion *sr = array_get(mark, i);
		Filerange r = view_regions_restore(&win->view, sr);
		if (text_range_valid(&r))
			array_add(&sel, &r);
	}
	vis_mark_normalize(&sel);
	return sel;
}

Array vis_mark_get(Win *win, enum VisMark id) {
	return mark_get(win, mark_from(win->vis, id));
}

static void mark_set(Win *win, Array *mark, Array *sel) {
	if (!mark)
		return;
	array_clear(mark);
	for (size_t i = 0, len = sel->len; i < len; i++) {
		SelectionRegion ss;
		Filerange *r = array_get(sel, i);
		if (view_regions_save(&win->view, r, &ss))
			array_add(mark, &ss);
	}
}

void vis_mark_set(Win *win, enum VisMark id, Array *sel) {
	mark_set(win, mark_from(win->vis, id), sel);
}

void vis_jumplist(Vis *vis, int advance)
{
	Win  *win  = vis->win;
	View *view = &win->view;
	Array cur = view_selections_get_all(view);

	size_t cursor = win->mark_set_lru_cursor;
	win->mark_set_lru_cursor += advance;
	if (advance < 0)
		cursor = win->mark_set_lru_cursor;
	cursor %= VIS_MARK_SET_LRU_COUNT;

	Array *next = win->mark_set_lru_regions + cursor;
	bool done = false;
	if (next->len) {
		Array sel = mark_get(win, next);
		done = vis_mark_equal(&sel, &cur);
		if (advance && !done) {
			/* NOTE: set cached selection */
			vis_mode_switch(vis, win->mark_set_lru_modes[cursor]);
			view_selections_set_all(view, &sel, view_selections_primary_get(view)->anchored);
		}
		array_release(&sel);
	}

	if (!advance && !done) {
		/* NOTE: save the current selection */
		mark_set(win, next, &cur);
		win->mark_set_lru_modes[cursor] = vis->mode->id;
		win->mark_set_lru_cursor++;
	}

	array_release(&cur);
}

enum VisMark vis_mark_from(Vis *vis, char mark) {
	if (mark >= 'a' && mark <= 'z')
		return VIS_MARK_a + mark - 'a';
	for (size_t i = 0; i < LENGTH(vis_marks); i++) {
		if (vis_marks[i].name == mark)
			return i;
	}
	return VIS_MARK_INVALID;
}

char vis_mark_to(Vis *vis, enum VisMark mark) {
	if (VIS_MARK_a <= mark && mark <= VIS_MARK_z)
		return 'a' + mark - VIS_MARK_a;

	if (mark < LENGTH(vis_marks))
		return vis_marks[mark].name;

	return '\0';
}

const MarkDef vis_marks[] = {
	[VIS_MARK_DEFAULT]        = { '\'', VIS_HELP("Default mark")    },
	[VIS_MARK_SELECTION]      = { '^',  VIS_HELP("Last selections") },
};
