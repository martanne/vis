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
	size_t len = array_length(a);
	if (len != array_length(b))
		return false;
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
	size_t len = array_length(mark);
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
	for (size_t i = 0, len = array_length(sel); i < len; i++) {
		SelectionRegion ss;
		Filerange *r = array_get(sel, i);
		if (view_regions_save(&win->view, r, &ss))
			array_add(mark, &ss);
	}
}

void vis_mark_set(Win *win, enum VisMark id, Array *sel) {
	mark_set(win, mark_from(win->vis, id), sel);
}

void marklist_init(MarkList *list, size_t max) {
	Array mark;
	mark_init(&mark);
	array_init_sized(&list->prev, sizeof(Array));
	array_reserve(&list->prev, max);
	array_add(&list->prev, &mark);
	array_init_sized(&list->next, sizeof(Array));
	array_reserve(&list->next, max);
}

void marklist_release(MarkList *list) {
	for (size_t i = 0, len = array_length(&list->prev); i < len; i++)
		array_release(array_get(&list->prev, i));
	array_release(&list->prev);
	for (size_t i = 0, len = array_length(&list->next); i < len; i++)
		array_release(array_get(&list->next, i));
	array_release(&list->next);
}

static bool marklist_push(Win *win, MarkList *list, Array *sel) {
	Array *top = array_peek(&list->prev);
	if (top) {
		Array top_sel = mark_get(win, top);
		bool eq = vis_mark_equal(&top_sel, sel);
		array_release(&top_sel);
		if (eq)
			return true;
	}

	for (size_t i = 0, len = array_length(&list->next); i < len; i++)
		array_release(array_get(&list->next, i));
	array_clear(&list->next);
	Array arr;
	mark_init(&arr);
	if (array_length(&list->prev) >= array_capacity(&list->prev)) {
		Array *tmp = array_get(&list->prev, 0);
		arr = *tmp;
		array_remove(&list->prev, 0);
	}
	mark_set(win, &arr, sel);
	return array_push(&list->prev, &arr);
}

bool vis_jumplist_save(Vis *vis) {
	Array sel = view_selections_get_all(&vis->win->view);
	bool ret = marklist_push(vis->win, &vis->win->jumplist, &sel);
	array_release(&sel);
	return ret;
}

static bool marklist_prev(Win *win, MarkList *list) {
	View *view = &win->view;
	bool restore = false;
	Array cur = view_selections_get_all(view);
	bool anchored = view_selections_primary_get(view)->anchored;
	Array *top = array_peek(&list->prev);
	if (!top)
		goto out;
	Array top_sel = mark_get(win, top);
	restore = !vis_mark_equal(&top_sel, &cur);
	if (restore)
		view_selections_set_all(view, &top_sel, anchored);
	array_release(&top_sel);
	if (restore)
		goto out;

	while (array_length(&list->prev) > 1) {
		Array *prev = array_pop(&list->prev);
		array_push(&list->next, prev);
		prev = array_peek(&list->prev);
		Array sel = mark_get(win, prev);
		restore = array_length(&sel) > 0;
		if (restore)
			view_selections_set_all(view, &sel, anchored);
		array_release(&sel);
		if (restore)
			goto out;
	}
out:
	array_release(&cur);
	return restore;
}

static bool marklist_next(Win *win, MarkList *list) {
	View *view = &win->view;
	bool anchored = view_selections_primary_get(view)->anchored;
	for (;;) {
		Array *next = array_pop(&list->next);
		if (!next)
			return false;
		Array sel = mark_get(win, next);
		if (array_length(&sel) > 0) {
			view_selections_set_all(view, &sel, anchored);
			array_release(&sel);
			array_push(&list->prev, next);
			return true;
		}
		array_release(next);
	}
}

bool vis_jumplist_prev(Vis *vis) {
	return marklist_prev(vis->win, &vis->win->jumplist);
}

bool vis_jumplist_next(Vis *vis) {
	return marklist_next(vis->win, &vis->win->jumplist);
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
