#include "util.h"

#include "tap.h"

#include "util.c"
#include "buffer.c"

static bool compare(Buffer *buf, const char *data, s64 len) {
	return buf->length == len && (len == 0 || memcmp(buf->data, data, buf->length) == 0);
}

static bool compare0(Buffer *b, str8 s) {
	return b->length == s.length && memcmp(b->data, s.data, b->length) == 0;
}

int main(int argc, char *argv[]) {
	Buffer buf = {0};

	plan_no_plan();

	ok(vis_buffer_insert(&buf, 0, "foo", 0) && buf.data == 0 && buf.length == 0 && buf.size == 0,
	   "Insert zero length data");
	ok(!vis_buffer_insert(&buf, 1, "foo", 3), "Insert string at invalid position");

	ok(vis_buffer_insert(&buf, 0, "", 0)    && compare0(&buf, str8("")),      "Insert empty string");
	ok(vis_buffer_insert(&buf, 0, "foo", 3) && compare0(&buf, str8("foo")),   "Insert string at start");
	ok(vis_buffer_insert(&buf, 1, "l", 1)   && compare0(&buf, str8("floo")),  "Insert string in middle");
	ok(vis_buffer_insert(&buf, 4, "r", 1)   && compare0(&buf, str8("floor")), "Insert string at end");

	ok(vis_buffer_put_str8(&buf, str8(""))    && compare0(&buf, str8("")),       "Put empty string");
	ok(vis_buffer_put_str8(&buf, str8("bar")) && compare0(&buf, str8("bar")),    "Put string");
	ok(vis_buffer_append0(&buf, "baz")        && compare0(&buf, str8("barbaz")), "Append string");

	buffer_release(&buf);
	ok(buf.data == 0 && buf.length == 0 && buf.size == 0, "Release");

	s64 cap = buf.size;

	ok(buffer_put(&buf, "foo", 0) && compare(&buf, "", 0),    "Put zero length data");
	ok(buffer_put(&buf, "bar", 3) && compare(&buf, "bar", 3), "Put data");

	vis_buffer_terminate(&buf);
	ok(buf.data[buf.length] == 0, "Terminate buffer");

	ok(buffer_append(&buf, "\0baz", 4) && compare(&buf, "bar\0baz", 7), "Append data");

	ok(buffer_grow(&buf, cap+1) && compare(&buf, "bar\0baz", 7) && buf.size >= cap+1, "Grow");
	buf.length = 0;

	skip_if(TIS_INTERPRETER, 1, "vsnprintf not supported") {
		bool append = true;
		for (int i = 1; i <= 10; i++)
			append &= vis_buffer_appendf(&buf, "%d", i);
		ok(append && compare0(&buf, str8("12345678910")), "Append formatted");
		buf.length = 0;

		append = true;
		for (int i = 1; i <= 10; i++)
			append &= vis_buffer_appendf(&buf, "%s", "");
		ok(append && compare0(&buf, str8("")), "Append formatted empty string");
		buf.length = 0;
	}

	buffer_release(&buf);

	return exit_status();
}
