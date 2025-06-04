#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "mojoshader.h"

#ifndef NUM_MACROS
#define NUM_MACROS 5
#endif

static const char *macro_names[NUM_MACROS] = { "A", "B", "C", "D", "E" };
static int macros_defined[NUM_MACROS];

typedef struct {
    char *data;
    int len;
    int cap;
} Buffer;

static void buf_init(Buffer *b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static int buf_append(Buffer *b, const char *fmt, ...)
{
    char tmp[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0)
        return 0;
    if (b->len + n >= b->cap)
    {
        int newcap = b->cap ? b->cap * 2 : 256;
        while (newcap <= b->len + n)
            newcap *= 2;
        char *ptr = (char *)realloc(b->data, newcap);
        if (!ptr)
            return 0;
        b->data = ptr;
        b->cap = newcap;
    }
    memcpy(b->data + b->len, tmp, n);
    b->len += n;
    return 1;
}

static void buf_free(Buffer *b)
{
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

static int count_lines(const char *data, int len)
{
    int lines = 0;
    for (int i = 0; i < len; i++)
        if (data[i] == '\n')
            lines++;
    return lines;
}

static int choose_macro(int want_defined)
{
    int indices[NUM_MACROS];
    int count = 0;
    for (int i = 0; i < NUM_MACROS; i++)
    {
        if (want_defined)
        {
            if (macros_defined[i])
                indices[count++] = i;
        }
        else
        {
            if (!macros_defined[i])
                indices[count++] = i;
        }
    }
    if (count == 0)
        return -1;
    return indices[rand() % count];
}

static void gen_plain(Buffer *b)
{
    buf_append(b, "int v%u;\n", (unsigned)rand());
}

static void gen_block(Buffer *b, int depth);

static void gen_conditional(Buffer *b, int depth)
{
    int condtrue = rand() % 2;
    int type = rand() % 3; /* 0: if, 1: ifdef, 2: ifndef */

    int idx;
    switch (type)
    {
        case 1: /* ifdef */
            idx = choose_macro(condtrue);
            if (idx < 0)
                buf_append(b, "#if %d\n", condtrue ? 1 : 0);
            else
                buf_append(b, "#ifdef %s\n", macro_names[idx]);
            break;
        case 2: /* ifndef */
            idx = choose_macro(!condtrue);
            if (idx < 0)
                buf_append(b, "#if %d\n", condtrue ? 1 : 0);
            else
                buf_append(b, "#ifndef %s\n", macro_names[idx]);
            break;
        default: /* if */
            buf_append(b, "#if %d\n", condtrue ? 1 : 0);
            break;
    }

    gen_block(b, depth + 1);
    if (rand() % 2)
    {
        buf_append(b, "#else\n");
        gen_block(b, depth + 1);
    }
    buf_append(b, "#endif\n");
}

static void gen_block(Buffer *b, int depth)
{
    int lines = (rand() % 3) + 1;
    for (int i = 0; i < lines; i++)
    {
        if ((rand() % 4) == 0 && depth < 2)
            gen_conditional(b, depth + 1);
        else
            gen_plain(b);
    }
}

static void reset_macros(void)
{
    for (int i = 0; i < NUM_MACROS; i++)
        macros_defined[i] = rand() % 2;
}

int main(int argc, char **argv)
{
    (void) argc; (void) argv;
    srand((unsigned int)time(NULL));
    for (int iter = 0; iter < 100; iter++)
    {
        Buffer buf; buf_init(&buf);
        reset_macros();
        for (int i = 0; i < NUM_MACROS; i++)
            if (macros_defined[i])
                buf_append(&buf, "#define %s\n", macro_names[i]);
        gen_block(&buf, 0);

        const MOJOSHADER_preprocessData *pd = MOJOSHADER_preprocess("fuzz", buf.data, buf.len, NULL, 0, NULL, NULL, NULL, NULL, NULL);
        if (pd->error_count > 0)
        {
            fprintf(stderr, "preprocessor error on iteration %d\n", iter);
            MOJOSHADER_freePreprocessData(pd);
            buf_free(&buf);
            return 1;
        }

        int in_lines = count_lines(buf.data, buf.len);
        int out_lines = count_lines(pd->output, pd->output_len);
        if (in_lines != out_lines)
        {
            fprintf(stderr, "line mismatch on iteration %d: %d vs %d\n", iter, in_lines, out_lines);
            MOJOSHADER_freePreprocessData(pd);
            buf_free(&buf);
            return 1;
        }
        MOJOSHADER_freePreprocessData(pd);
        buf_free(&buf);
    }
    printf("fuzz tests passed\n");
    return 0;
}

/* end of fuzzpp.c */

