#include "util.h"

#include "tap.h"

#include "buffer.c"

static bool compare(Buffer *buf, const char *data, size_t len) {
	return buf->len == len && (len == 0 || memcmp(buf->data, data, buf->len) == 0);
}

static bool compare0(Buffer *buf, const char *data) {
	return buf->len == strlen(data)+1 && memcmp(buf->data, data, buf->len) == 0;
}

int main(int argc, char *argv[]) {
	Buffer buf = {0};

	plan_no_plan();

	ok(buf.data == NULL && buf.len == 0 && buf.size == 0, "Initialization");
	ok(buffer_insert(&buf, 0, "foo", 0) && buf.data == NULL &&
	   buf.len == 0 && buf.size == 0, "Insert zero length data");
	ok(!buffer_insert0(&buf, 1, "foo"), "Insert string at invalid position");

	ok(buffer_insert0(&buf, 0, "") && compare0(&buf, ""), "Insert empty string");
	ok(buffer_insert0(&buf, 0, "foo") && compare0(&buf, "foo"), "Insert string at start");
	ok(buffer_insert0(&buf, 1, "l") && compare0(&buf, "floo"), "Insert string in middle");
	ok(buffer_insert0(&buf, 4, "r") && compare0(&buf, "floor"), "Insert string at end");

	ok(buffer_put0(&buf, "") && compare0(&buf, ""), "Put empty string");
	ok(buffer_put0(&buf, "bar") && compare0(&buf, "bar"), "Put string");

	ok(buffer_append0(&buf, "baz") && compare0(&buf, "barbaz"), "Append string");

	buffer_release(&buf);
	ok(buf.data == NULL && buf.len == 0 && buf.size == 0, "Release");

	ok(buffer_insert(&buf, 0, "foo", 0) && compare(&buf, "", 0), "Insert zero length data");
	ok(buffer_insert(&buf, 0, "foo", 3) && compare(&buf, "foo", 3), "Insert data at start");
	ok(buffer_insert(&buf, 1, "l", 1) && compare(&buf, "floo", 4), "Insert data in middle");
	ok(buffer_insert(&buf, 4, "r", 1) && compare(&buf, "floor", 5), "Insert data at end");

	size_t cap = buf.size;
	buf.len = 0;
	ok(buf.data && buf.len == 0 && buf.size == cap, "Clear");

	ok(buffer_put(&buf, "foo", 0) && compare(&buf, "", 0), "Put zero length data");
	ok(buffer_put(&buf, "bar", 3) && compare(&buf, "bar", 3), "Put data");

	ok(buffer_append(&buf, "\0baz", 4) && compare(&buf, "bar\0baz", 7), "Append data");

	ok(buffer_grow(&buf, cap+1) && compare(&buf, "bar\0baz", 7) && buf.size >= cap+1, "Grow");
	buf.len = 0;

	skip_if(TIS_INTERPRETER, 1, "vsnprintf not supported") {
		bool append = true;
		for (int i = 1; i <= 10; i++)
			append &= buffer_appendf(&buf, "%d", i);
		ok(append && compare0(&buf, "12345678910"), "Append formatted");
		buf.len = 0;

		append = true;
		for (int i = 1; i <= 10; i++)
			append &= buffer_appendf(&buf, "%s", "");
		ok(append && compare0(&buf, ""), "Append formatted empty string");
		buf.len = 0;
	}

	buffer_release(&buf);

	return exit_status();
}
