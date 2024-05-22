#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tap.h"
#include "buffer.h"

static bool compare(Buffer *buf, const char *data, size_t len) {
	return buf->len == len && (len == 0 || memcmp(buf->data, data, buf->len) == 0);
}

static bool compare0(Buffer *buf, const char *data) {
	return buf->len == strlen(data)+1 && memcmp(buf->data, data, buf->len) == 0;
}

int main(int argc, char *argv[]) {
	Buffer buf;

	plan_no_plan();

	buffer_init(&buf);
	ok(buffer_content(&buf) == NULL && buffer_length(&buf) == 0 && buffer_capacity(&buf) == 0, "Initialization");
	ok(buffer_insert(&buf, 0, "foo", 0) && buffer_content(&buf) == NULL &&
	   buffer_length(&buf) == 0 && buffer_capacity(&buf) == 0, "Insert zero length data");
	ok(!buffer_insert0(&buf, 1, "foo"), "Insert string at invalid position");

	ok(buffer_insert0(&buf, 0, "") && compare0(&buf, ""), "Insert empty string");
	ok(buffer_insert0(&buf, 0, "foo") && compare0(&buf, "foo"), "Insert string at start");
	ok(buffer_insert0(&buf, 1, "l") && compare0(&buf, "floo"), "Insert string in middle");
	ok(buffer_insert0(&buf, 4, "r") && compare0(&buf, "floor"), "Insert string at end");

	ok(buffer_put0(&buf, "") && compare0(&buf, ""), "Put empty string");
	ok(buffer_put0(&buf, "bar") && compare0(&buf, "bar"), "Put string");

	ok(buffer_prepend0(&buf, "foo") && compare0(&buf, "foobar"), "Prepend string");
	ok(buffer_append0(&buf, "baz") && compare0(&buf, "foobarbaz"), "Append string");

	buffer_release(&buf);
	ok(buf.data == NULL && buffer_length(&buf) == 0 && buffer_capacity(&buf) == 0, "Release");

	ok(buffer_insert(&buf, 0, "foo", 0) && compare(&buf, "", 0), "Insert zero length data");
	ok(buffer_insert(&buf, 0, "foo", 3) && compare(&buf, "foo", 3), "Insert data at start");
	ok(buffer_insert(&buf, 1, "l", 1) && compare(&buf, "floo", 4), "Insert data in middle");
	ok(buffer_insert(&buf, 4, "r", 1) && compare(&buf, "floor", 5), "Insert data at end");

	size_t cap = buffer_capacity(&buf);
	buffer_clear(&buf);
	ok(buf.data && buffer_length(&buf) == 0 && buffer_capacity(&buf) == cap, "Clear");

	ok(buffer_put(&buf, "foo", 0) && compare(&buf, "", 0), "Put zero length data");
	ok(buffer_put(&buf, "bar", 3) && compare(&buf, "bar", 3), "Put data");

	ok(buffer_prepend(&buf, "foo\0", 4) && compare(&buf, "foo\0bar", 7), "Prepend data");
	ok(buffer_append(&buf, "\0baz", 4) && compare(&buf, "foo\0bar\0baz", 11), "Append data");

	ok(buffer_grow(&buf, cap+1) && compare(&buf, "foo\0bar\0baz", 11) && buffer_capacity(&buf) >= cap+1, "Grow");

	const char *content = buffer_content(&buf);
	char *data = buffer_move(&buf);
	ok(data == content && buffer_length(&buf) == 0 && buffer_capacity(&buf) == 0 && buffer_content(&buf) == NULL, "Move");
	ok(buffer_append0(&buf, "foo") && buffer_content(&buf) != data, "Modify after move");
	free(data);

	skip_if(TIS_INTERPRETER, 1, "vsnprintf not supported") {

		ok(buffer_printf(&buf, "Test: %d\n", 42) && compare0(&buf, "Test: 42\n"), "Set formatted");
		ok(buffer_printf(&buf, "%d\n", 42) && compare0(&buf, "42\n"), "Set formatted overwrite");
		buffer_clear(&buf);

		ok(buffer_printf(&buf, "%s", "") && compare0(&buf, ""), "Set formatted empty string");
		buffer_clear(&buf);

		bool append = true;
		for (int i = 1; i <= 10; i++)
			append &= buffer_appendf(&buf, "%d", i);
		ok(append && compare0(&buf, "12345678910"), "Append formatted");
		buffer_clear(&buf);

		append = true;
		for (int i = 1; i <= 10; i++)
			append &= buffer_appendf(&buf, "%s", "");
		ok(append && compare0(&buf, ""), "Append formatted empty string");
		buffer_clear(&buf);
	}

	buffer_release(&buf);

	return exit_status();
}
