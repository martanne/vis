#include <ccan/tap/tap.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "buffer.h"

static bool compare(Buffer *buf, const char *data, size_t len) {
	return buf->len == len && memcmp(buf->data, data, buf->len) == 0;
}

static bool compare0(Buffer *buf, const char *data) {
	return buf->len == strlen(data)+1 && memcmp(buf->data, data, buf->len) == 0;
}

int main(int argc, char *argv[]) {
	Buffer buf;

	plan_no_plan();

	buffer_init(&buf);
	ok(buf.data == NULL && buf.len == 0 && buf.size == 0, "Initialization");
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
	ok(buf.data == NULL && buf.len == 0 && buf.size == 0, "Release");

	ok(buffer_insert(&buf, 0, "foo", 0) && compare(&buf, "", 0), "Insert zero length data");
	ok(buffer_insert(&buf, 0, "foo", 3) && compare(&buf, "foo", 3), "Insert data at start");
	ok(buffer_insert(&buf, 1, "l", 1) && compare(&buf, "floo", 4), "Insert data in middle");
	ok(buffer_insert(&buf, 4, "r", 1) && compare(&buf, "floor", 5), "Insert data at end");

	size_t size = buf.size;
	buffer_truncate(&buf);
	ok(buf.len == 0 && buf.data && buf.size == size, "Truncate");

	ok(buffer_put(&buf, "foo", 0) && compare(&buf, "", 0), "Put zero length data");
	ok(buffer_put(&buf, "bar", 3) && compare(&buf, "bar", 3), "Put data");

	ok(buffer_prepend(&buf, "foo\0", 4) && compare(&buf, "foo\0bar", 7), "Prepend data");
	ok(buffer_append(&buf, "\0baz", 4) && compare(&buf, "foo\0bar\0baz", 11), "Append data");

	ok(buffer_grow(&buf, size+1) && compare(&buf, "foo\0bar\0baz", 11) && buf.size >= size+1, "Grow");

	size = buf.size;
	buffer_clear(&buf);
	ok(buf.len == 0 && buf.data && buf.size == size, "Clear");

	return exit_status();
}
