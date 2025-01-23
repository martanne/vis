#include <stdlib.h>
#include <string.h>

#include "text-regex.h"

struct Regex {
	regex_t regex;
};

Regex *text_regex_new(void) {
	Regex *r = calloc(1, sizeof(Regex));
	if (!r)
		return NULL;
	regcomp(&r->regex, "\0\0", 0); /* this should not match anything */
	return r;
}

int text_regex_compile(Regex *regex, const char *string, size_t len, int cflags) {
	int r = regcomp(&regex->regex, string, cflags);
	if (r)
		regcomp(&regex->regex, "\0\0", 0);
	return r;
}

size_t text_regex_nsub(Regex *r) {
	if (!r)
		return 0;
	return r->regex.re_nsub;
}

void text_regex_free(Regex *r) {
	if (!r)
		return;
	regfree(&r->regex);
	free(r);
}

int text_regex_match(Regex *r, const char *data, int eflags) {
	return regexec(&r->regex, data, 0, NULL, eflags);
}

int text_search_range_forward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
	char *buf = text_bytes_alloc0(txt, pos, len);
	if (!buf)
		return REG_NOMATCH;
	char *cur = buf, *end = buf + len;
	int ret = REG_NOMATCH;
	regmatch_t match[MAX_REGEX_SUB];
	for (size_t junk = len; len > 0; len -= junk, pos += junk) {
		ret = regexec(&r->regex, cur, nmatch, match, eflags);
		if (!ret) {
			for (size_t i = 0; i < nmatch; i++) {
				pmatch[i].start = match[i].rm_so == -1 ? EPOS : pos + match[i].rm_so;
				pmatch[i].end = match[i].rm_eo == -1 ? EPOS : pos + match[i].rm_eo;
			}
			break;
		}
		char *next = memchr(cur, 0, len);
		if (!next)
			break;
		while (!*next && next != end)
			next++;
		junk = next - cur;
		cur = next;
	}
	free(buf);
	return ret;
}

int text_search_range_backward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
	char *buf = text_bytes_alloc0(txt, pos, len);
	if (!buf)
		return REG_NOMATCH;
	char *cur = buf, *end = buf + len;
	int ret = REG_NOMATCH;
	regmatch_t match[MAX_REGEX_SUB];
	for (size_t junk = len; len > 0; len -= junk, pos += junk) {
		char *next;
		if (!regexec(&r->regex, cur, nmatch, match, eflags)) {
			ret = 0;
			for (size_t i = 0; i < nmatch; i++) {
				pmatch[i].start = match[i].rm_so == -1 ? EPOS : pos + match[i].rm_so;
				pmatch[i].end = match[i].rm_eo == -1 ? EPOS : pos + match[i].rm_eo;
			}

			if (match[0].rm_so == 0 && match[0].rm_eo == 0) {
				/* empty match at the beginning of cur, advance to next line */
				next = strchr(cur, '\n');
				if (!next)
					break;
				next++;
			} else {
				next = cur + match[0].rm_eo;
			}
		} else {
			next = memchr(cur, 0, len);
			if (!next)
				break;
			while (!*next && next != end)
				next++;
		}
		junk = next - cur;
		cur = next;
		if (cur[-1] == '\n')
			eflags &= ~REG_NOTBOL;
		else
			eflags |= REG_NOTBOL;
	}
	free(buf);
	return ret;
}
