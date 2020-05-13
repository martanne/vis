#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "tap.h"
#include "text.h"
#include "text-util.h"
#include "util.h"

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

static bool insert(Text *txt, size_t pos, const char *data) {
	return text_insert(txt, pos, data, strlen(data));
}

static bool isempty(Text *txt) {
	return text_size(txt) == 0;
}

static bool compare_iterator_forward(Iterator *it, const char *data) {
	char buf[BUFSIZ] = "", b;
	while (text_iterator_byte_get(it, &b)) {
		buf[it->pos] = b;
		text_iterator_byte_next(it, NULL);
	}
	return strcmp(data, buf) == 0;
}

static bool compare_iterator_backward(Iterator *it, const char *data) {
	char buf[BUFSIZ] = "", b;
	while (text_iterator_byte_get(it, &b)) {
		buf[it->pos] = b;
		text_iterator_byte_prev(it, NULL);
	}
	return strcmp(data, buf) == 0;
}

static bool compare_iterator_both(Text *txt, const char *data) {
	Iterator it = text_iterator_get(txt, 0);
	bool forward = compare_iterator_forward(&it, data);
	text_iterator_byte_prev(&it, NULL);
	bool forward_backward = compare_iterator_backward(&it, data);
	it = text_iterator_get(txt, text_size(txt));
	bool backward = compare_iterator_backward(&it, data);
	text_iterator_byte_next(&it, NULL);
	bool backward_forward = compare_iterator_forward(&it, data);
	return forward && backward && forward_backward && backward_forward;
}

static bool compare(Text *txt, const char *data) {
	char buf[BUFSIZ];
	size_t len = text_bytes_get(txt, 0, sizeof(buf)-1, buf);
	buf[len] = '\0';
	return len == strlen(data) && strcmp(buf, data) == 0 &&
	       compare_iterator_both(txt, data);
}

static void iterator_find_everywhere(Text *txt, char *data) {
	size_t len = strlen(data);

	Iterator it = text_iterator_get(txt, 0);

	for (size_t i = 0; i < len; i++) {
		ok(text_iterator_byte_find_next(&it, data[i]) && it.pos == i && text_iterator_byte_next(&it, NULL) && it.pos == i+1, "Iterator find byte next at current position");
	}
	ok(!text_iterator_byte_find_next(&it, data[len-1]) && it.pos == len, "Iterator find byte next at EOF");

	for (size_t i = len; i-- > 0;) {
		ok(text_iterator_byte_find_prev(&it, data[i]) && it.pos == i, "Iterator find byte prev at current position");
	}
	ok(!text_iterator_byte_find_prev(&it, data[0]) && it.pos == 0, "Iterator find byte prev at BOF");

}

static void iterator_find_next(Text *txt, size_t start, char b, size_t match) {
	Iterator it = text_iterator_get(txt, start);
	bool found = text_iterator_byte_find_next(&it, b);
	ok((found && it.pos == match) || (!found && it.pos == text_size(txt)),"Iterator byte find next (start: %zu, match: %zu)", start, match);
}

static void iterator_find_prev(Text *txt, size_t start, char b, size_t match) {
	Iterator it = text_iterator_get(txt, start);
	bool found = text_iterator_byte_find_prev(&it, b);
	ok((found && it.pos == match) || (!found && it.pos == 0),"Iterator byte find prev (start: %zu, match: %zu)", start, match);
}

int main(int argc, char *argv[]) {
	Text *txt;

	plan_no_plan();

	skip_if(TIS_INTERPRETER, 2, "I/O related") {

		const char *filename = "data";

		enum TextLoadMethod load_method[] = {
			TEXT_LOAD_AUTO,
			TEXT_LOAD_READ,
			TEXT_LOAD_MMAP,
		};

		for (size_t i = 0; i < LENGTH(load_method); i++) {
			txt = text_load_method("/", load_method[i]);
			ok(txt == NULL && errno == EISDIR, "Opening directory (method %zu)", i);

			if (access("/etc/shadow", F_OK) == 0 && access("/etc/shadow", R_OK) != 0) {
				txt = text_load_method("/etc/shadow", load_method[i]);
				ok(txt == NULL && errno == EACCES, "Opening file without sufficient permissions (method %zu)", i);
			}
		}

		char buf[BUFSIZ] = "Hello World!\n";
		txt = text_load(NULL);
		ok(txt && insert(txt, 0, buf) && compare(txt, buf), "Inserting into empty text");
		ok(txt && text_save(txt, filename), "Text save");
		text_free(txt);

		for (size_t i = 0; i < LENGTH(load_method); i++) {
			txt = text_load_method(filename, load_method[i]);
			ok(txt && compare(txt, buf), "Load text (method %zu)", i);
			text_free(txt);
		}

		enum TextSaveMethod save_method[] = {
			TEXT_SAVE_AUTO,
			TEXT_SAVE_ATOMIC,
			TEXT_SAVE_INPLACE,
		};

		for (size_t i = 0; i < LENGTH(save_method); i++) {
			snprintf(buf, sizeof buf, "Hello World: %zu\n", i);
			txt = text_load(NULL);
			ok(txt && insert(txt, 0, buf) && compare(txt, buf), "Preparing to save (method %zu)", i);
			ok(txt && text_save_method(txt, filename, save_method[i]), "Text save (method %zu)", i);
			text_free(txt);

			txt = text_load(filename);
			ok(txt && compare(txt, buf), "Verify save (method %zu)", i);
			text_free(txt);
		}
	}

	txt = text_load(NULL);
	ok(txt != NULL && isempty(txt), "Opening empty file");

	Iterator it = text_iterator_get(txt, 0);
	ok(text_iterator_valid(&it) && it.pos == 0, "Iterator on empty file");
	char b = '_';
	ok(text_iterator_byte_get(&it, &b) && b == '\0', "Read EOF from iterator of empty file");
	b = '_';
	ok(!text_iterator_byte_prev(&it, &b) && b == '_' &&
	   !text_iterator_valid(&it), "Moving iterator beyond start of file");
	ok(!text_iterator_byte_get(&it, &b) && b == '_' &&
	   !text_iterator_valid(&it), "Access iterator beyond start of file");
	ok(text_iterator_byte_next(&it, &b) && b == '\0' &&
	   text_iterator_valid(&it), "Moving iterator back from beyond start of file");
	b = '_';
	ok(text_iterator_byte_get(&it, &b) && b == '\0' &&
	   text_iterator_valid(&it), "Accessing iterator after moving back from beyond start of file");
	b = '_';
	ok(!text_iterator_byte_next(&it, &b) && b == '_' &&
	   !text_iterator_valid(&it), "Moving iterator beyond end of file");
	ok(!text_iterator_byte_get(&it, &b) && b == '_' &&
	   !text_iterator_valid(&it), "Accessing iterator beyond end of file");
	ok(text_iterator_byte_prev(&it, &b) && b == '\0' &&
	   text_iterator_valid(&it), "Moving iterator back from beyond end of file");
	b = '_';
	ok(text_iterator_byte_get(&it, &b) && b == '\0' &&
	   text_iterator_valid(&it), "Accessing iterator after moving back from beyond start of file");

	char data[] = "a\nb\nc\n";
	size_t data_len = strlen(data);
	ok(insert(txt, 0, data), "Inserting new lines");
	iterator_find_everywhere(txt, data);
	iterator_find_next(txt, 0, 'a', 0);
	iterator_find_next(txt, 0, 'b', 2);
	iterator_find_next(txt, 0, 'c', 4);
	iterator_find_next(txt, 0, 'e', EPOS);
	iterator_find_prev(txt, data_len, 'a', 0);
	iterator_find_prev(txt, data_len, 'b', 2);
	iterator_find_prev(txt, data_len, 'c', 4);
	iterator_find_prev(txt, data_len, 'e', EPOS);
	ok(text_undo(txt) == 0 && isempty(txt), "Undo to empty document 1");

	ok(insert(txt, 1, "") && isempty(txt), "Inserting empty data");
	ok(!insert(txt, 1, " ") && isempty(txt), "Inserting with invalid offset");

	/* test cached insertion (i.e. in-place with only one piece) */
	ok(insert(txt, 0, "3") && compare(txt, "3"), "Inserting into empty document (cached)");
	ok(insert(txt, 0, "1") && compare(txt, "13"), "Inserting at begin (cached)");
	ok(insert(txt, 1, "2") && compare(txt, "123"), "Inserting in middle (cached)");
	ok(insert(txt, text_size(txt), "4") && compare(txt, "1234"), "Inserting at end (cached)");

	ok(text_delete(txt, text_size(txt), 0) && compare(txt, "1234"), "Deleting empty range");
	ok(!text_delete(txt, text_size(txt), 1) && compare(txt, "1234"), "Deleting invalid offset");
	ok(!text_delete(txt, 0, text_size(txt)+5) && compare(txt, "1234"), "Deleting invalid range");

	ok(text_undo(txt) == 0 && compare(txt, ""), "Reverting to empty document");
	ok(text_redo(txt) != EPOS /* == text_size(txt) */ && compare(txt, "1234"), "Restoring previsous content");

	/* test cached deletion (i.e. in-place with only one piece) */
	ok(text_delete(txt, text_size(txt)-1, 1) && compare(txt, "123"), "Deleting at end (cached)");
	ok(text_delete(txt, 1, 1) && compare(txt, "13"), "Deleting in middle (cached)");
	ok(text_delete(txt, 0, 1) && compare(txt, "3"), "Deleting at begin (cached)");
	ok(text_delete(txt, 0, 1) && compare(txt, ""), "Deleting to empty document (cached)");

	/* test regular insertion (i.e. with multiple pieces) */
	text_snapshot(txt);
	ok(insert(txt, 0, "3") && compare(txt, "3"), "Inserting into empty document");
	text_snapshot(txt);
	ok(insert(txt, 0, "1") && compare(txt, "13"), "Inserting at begin");
	text_snapshot(txt);
	ok(insert(txt, 1, "2") && compare(txt, "123"), "Inserting in between");
	text_snapshot(txt);
	ok(insert(txt, text_size(txt), "46") && compare(txt, "12346"), "Inserting at end");
	text_snapshot(txt);
	ok(insert(txt, 4, "5") && compare(txt, "123456"), "Inserting in middle");
	text_snapshot(txt);
	ok(insert(txt, text_size(txt), "789") && compare(txt, "123456789"), "Inserting at end");
	text_snapshot(txt);
	ok(insert(txt, text_size(txt), "0") && compare(txt, "1234567890"), "Inserting at end");

	/* test simple undo / redo oparations */
	ok(text_undo(txt) != EPOS && compare(txt, "123456789"), "Undo 1");
	ok(text_undo(txt) != EPOS && compare(txt, "123456"), "Undo 2");
	ok(text_undo(txt) != EPOS && compare(txt, "12346"), "Undo 3");
	ok(text_undo(txt) != EPOS && compare(txt, "123"), "Undo 3");
	ok(text_undo(txt) != EPOS && compare(txt, "13"), "Undo 5");
	ok(text_undo(txt) != EPOS && compare(txt, "3"), "Undo 6");
	ok(text_undo(txt) != EPOS && compare(txt, ""), "Undo 6");
	ok(text_redo(txt) != EPOS && compare(txt, "3"), "Redo 1");
	ok(text_redo(txt) != EPOS && compare(txt, "13"), "Redo 2");
	ok(text_redo(txt) != EPOS && compare(txt, "123"), "Redo 3");
	ok(text_redo(txt) != EPOS && compare(txt, "12346"), "Redo 4");
	ok(text_redo(txt) != EPOS && compare(txt, "123456"), "Redo 5");
	ok(text_redo(txt) != EPOS && compare(txt, "123456789"), "Redo 6");
	ok(text_redo(txt) != EPOS && compare(txt, "1234567890"), "Redo 7");

	/* test regular deletion (i.e. with multiple pieces) */
	ok(text_delete(txt, 8, 2) && compare(txt, "12345678"), "Deleting midway start");
	text_undo(txt);
	ok(text_delete(txt, 2, 6) && compare(txt, "1290"), "Deleting midway end");
	text_undo(txt);
	ok(text_delete(txt, 7, 1) && compare(txt, "123456790"), "Deleting midway both same piece");
	text_undo(txt);
	ok(text_delete(txt, 0, 5) && compare(txt, "67890"), "Deleting at begin");
	text_undo(txt);
	ok(text_delete(txt, 5, 5) && compare(txt, "12345"), "Deleting at end");

	ok(text_mark_get(txt, text_mark_set(txt, -1)) == EPOS, "Mark invalid 1");
	ok(text_mark_get(txt, text_mark_set(txt, text_size(txt)+1)) == EPOS, "Mark invalid 2");

	const char *chunk = "new content";
	const size_t delta = strlen(chunk);
	size_t positions[] = { 0, 1, text_size(txt)/2, text_size(txt)-1 };
	text_snapshot(txt);
	for (size_t i = 0; i < LENGTH(positions); i++) {
		size_t pos = positions[i];
		Mark bof = text_mark_set(txt, 0);
		ok(text_mark_get(txt, bof) == 0, "Mark at beginning of file");
		Mark mof = text_mark_set(txt, pos);
		ok(text_mark_get(txt, mof) == pos, "Mark in the middle");
		Mark eof = text_mark_set(txt, text_size(txt));
		ok(text_mark_get(txt, eof) == text_size(txt), "Mark at end of file");
		ok(insert(txt, pos, chunk), "Insert before mark");
		ok(text_mark_get(txt, bof) == ((pos == 0) ? delta : 0), "Mark at beginning adjusted 1");
		ok(text_mark_get(txt, mof) == pos+delta, "Mark in the middle adjusted 1");
		ok(text_mark_get(txt, eof) == text_size(txt), "Mark at end adjusted 1");
		ok(insert(txt, pos+delta+1, chunk), "Insert after mark");
		ok(text_mark_get(txt, bof) == ((pos == 0) ? delta : 0), "Mark at beginning adjusted 2");
		ok(text_mark_get(txt, mof) == pos+delta, "Mark in the middle adjusted 2");
		ok(text_mark_get(txt, eof) == text_size(txt), "Mark at end adjusted 2");
		text_snapshot(txt);
		ok(text_delete(txt, pos+delta, 1), "Deleting mark");
		ok(text_mark_get(txt, mof) == EPOS, "Mark in the middle deleted");
		text_undo(txt);
		ok(text_mark_get(txt, mof) == pos+delta, "Mark restored");
		text_undo(txt);
	}
	text_free(txt);

	return exit_status();
}
