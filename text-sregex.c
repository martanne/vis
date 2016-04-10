#include <stdlib.h>
#include <string.h>
#include <sregex.h>

#include "text-regex.h"

struct Regex {
    sre_pool_t *pool; // pool for parsing and compiling
    sre_pool_t *mpool; // pool for matching
    sre_regex_t *regex;
    sre_program_t *prog;
    sre_uint_t ncaptures;
    sre_int_t* captures;
};

Regex *text_regex_new(void) {
    Regex* result = calloc(1, sizeof(Regex));
    return result;
}

static sre_uint_t regex_capture_size(Regex *r) {
    return 2 * (r->ncaptures + 1) * sizeof(sre_int_t);
}

static int translate_cflags(int cflags) {
    return 0;
}

int text_regex_compile(Regex *r, const char *string, int cflags) {
    sre_int_t err_offset;
    r->pool = sre_create_pool(4096);
    r->regex = sre_regex_parse(r->pool, (sre_char *) string, &r->ncaptures, translate_cflags(cflags), &err_offset);

    if (!r->regex) {
        sre_destroy_pool(r->pool);
        return 1;
    }

    r->prog = sre_regex_compile(r->pool, r->regex);
    r->captures = malloc(regex_capture_size(r));
    r->mpool = sre_create_pool(4096);
    return 0;
}

void text_regex_free(Regex *r) {
    if (r) {
        if (r->pool) sre_destroy_pool(r->pool);
        if (r->mpool) sre_destroy_pool(r->mpool);
        free(r);
    }
}

static sre_vm_pike_ctx_t *make_context(Regex *r) {
    return sre_vm_pike_create_ctx(r->mpool, r->prog, r->captures, regex_capture_size(r));
}

static void destroy_context(sre_vm_pike_ctx_t *ctx, Regex *r) {
    sre_reset_pool(r->mpool);
}

int text_regex_match(Regex *r, const char *data, int eflags) {
    sre_int_t *unused;
    sre_vm_pike_ctx_t *re_ctx = make_context(r);
    sre_int_t result = sre_vm_pike_exec(re_ctx, (sre_char *) data, strlen(data), 1, &unused);
    destroy_context(re_ctx, r);
    return result >= 0;
}

static void fill_match(Regex *r, size_t nmatch, RegexMatch pmatch[], size_t offset) {
    sre_int_t *cap = r->captures;
    for (size_t i = 0; i < nmatch; i++) {
        pmatch[i].start = *cap == -1 ? EPOS : *cap + offset;
        ++cap;
        pmatch[i].end = *cap == -1 ? EPOS : *cap + offset;
        ++cap;
    }
}

int text_search_range_forward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
    int ret = 0;
    sre_int_t *unused;
    sre_vm_pike_ctx_t *re_ctx = make_context(r);

    text_iterate(txt, it, pos) {
        sre_int_t result = sre_vm_pike_exec(re_ctx, (sre_char *) it.text, it.end - it.text, text_iterator_at_end(&it) ? 1 : 0, &unused);
        if (result == SRE_DECLINED || result == SRE_ERROR) {
            ret = REG_NOMATCH;
            break;
        }
        
        if (result >= 0) {
            fill_match(r, nmatch, pmatch, pos);
            ret = 0;
            break;
        }
    }

    destroy_context(re_ctx, r);
    return ret;
}

int text_search_range_backward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
    sre_int_t *unused;
    int ret = REG_NOMATCH;

    text_iterate(txt, it, pos) {
        while (it.text != it.end) {
            sre_vm_pike_ctx_t *re_ctx = make_context(r);
            sre_int_t result = sre_vm_pike_exec(re_ctx, (sre_char *) it.text, it.end - it.text, text_iterator_at_end(&it) ? 1 : 0, &unused);
            destroy_context(re_ctx, r);

            if (result == SRE_DECLINED || result == SRE_ERROR)
                return REG_NOMATCH;

            if (result == SRE_AGAIN)
                break;
            
            ret = 0;
            fill_match(r, nmatch, pmatch, it.pos);

            // TODO Handle empty match by skipping to next line
            text_iterator_skip_bytes(&it, r->captures[1]);
        }
    }

    return ret;
}
