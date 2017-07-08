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

void marks_init(Array *arr) {
	array_init_sized(arr, sizeof(SelectionRegion));
}

void mark_release(Array *arr) {
	if (!arr)
		return;
	array_release(arr);
}


static Array *mark_from(Vis *vis, enum VisMark id) {
	if (id == VIS_MARK_SELECTION && vis->win)
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

Array vis_mark_get(Vis *vis, enum VisMark id) {
	Array sel;
	array_init_sized(&sel, sizeof(Filerange));
	Array *mark = mark_from(vis, id);
	if (!mark)
		return sel;
	View *view = vis->win->view;
	size_t len = array_length(mark);
	array_reserve(&sel, len);
	for (size_t i = 0; i < len; i++) {
		SelectionRegion *sr = array_get(mark, i);
		Filerange r = view_regions_restore(view, sr);
		if (text_range_valid(&r))
			array_add(&sel, &r);
	}
	vis_mark_normalize(&sel);
	return sel;
}

void vis_mark_set(Vis *vis, enum VisMark id, Array *sel) {
	Array *mark = mark_from(vis, id);
	if (!mark)
		return;
	array_clear(mark);
	View *view = vis->win->view;
	for (size_t i = 0, len = array_length(sel); i < len; i++) {
		SelectionRegion ss;
		Filerange *r = array_get(sel, i);
		if (view_regions_save(view, r, &ss))
			array_add(mark, &ss);
	}
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

const MarkDef vis_marks[] = {
	[VIS_MARK_DEFAULT]        = { '"', VIS_HELP("Default mark")    },
	[VIS_MARK_SELECTION]      = { '^', VIS_HELP("Last selections") },
};
