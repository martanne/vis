#include <stdlib.h>
#include <string.h>
#include <regex.h>

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

int text_regex_compile(Regex *regex, const char *string, int cflags) {
	int r = regcomp(&regex->regex, string, cflags);
	if (r)
		regcomp(&regex->regex, "\0\0", 0);
	return r;
}

void text_regex_free(Regex *r) {
	if (!r)
		return;
	regfree(&r->regex);
	free(r);
}

int text_search_range_forward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
	char *buf = text_bytes_alloc0(txt, pos, len);
	if (!buf)
		return REG_NOMATCH;
	regmatch_t match[nmatch];
	int ret = regexec(&r->regex, buf, nmatch, match, eflags);
	if (!ret) {
		for (size_t i = 0; i < nmatch; i++) {
			pmatch[i].start = match[i].rm_so == -1 ? EPOS : pos + match[i].rm_so;
			pmatch[i].end = match[i].rm_eo == -1 ? EPOS : pos + match[i].rm_eo;
		}
	}
	free(buf);
	return ret;
}

int text_search_range_backward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
	char *buf = text_bytes_alloc0(txt, pos, len);
	if (!buf)
		return REG_NOMATCH;
	regmatch_t match[nmatch];
	char *cur = buf;
	int ret = REG_NOMATCH;
	while (!regexec(&r->regex, cur, nmatch, match, eflags)) {
		ret = 0;
		for (size_t i = 0; i < nmatch; i++) {
			pmatch[i].start = match[i].rm_so == -1 ? EPOS : pos + (size_t)(cur - buf) + match[i].rm_so;
			pmatch[i].end = match[i].rm_eo == -1 ? EPOS : pos + (size_t)(cur - buf) + match[i].rm_eo;
		}
		if (match[0].rm_so == 0 && match[0].rm_eo == 0) {
			/* empty match at the beginning of cur, advance to next line */
			if ((cur = strchr(cur, '\n')))
				cur++;
			else
				break;

		} else {
			cur += match[0].rm_eo;
		}
	}
	free(buf);
	return ret;
}
