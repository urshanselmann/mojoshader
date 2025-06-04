#include <stdio.h>
#include <stdlib.h>
#include "mojoshader.h"

static char *readfile(const char *fname, int *len)
{
    FILE *f = fopen(fname, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size ? size : 1);
    if (!buf) { fclose(f); return NULL; }
    if (size && fread(buf, size, 1, f) != 1) { fclose(f); free(buf); return NULL; }
    fclose(f);
    *len = (int)size;
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "Usage: %s <file>\n", argv[0]); return 1; }
    int len = 0; char *src = readfile(argv[1], &len);
    if (!src) { fprintf(stderr, "Failed to read %s\n", argv[1]); return 1; }
    const MOJOSHADER_preprocessData *pd = MOJOSHADER_preprocess(argv[1], src, len, NULL, 0, NULL, NULL, NULL, NULL, NULL);
    free(src);
    if (pd->error_count > 0) {
        int i; for (i = 0; i < pd->error_count; i++)
            fprintf(stderr, "%s:%d: ERROR: %s\n", pd->errors[i].filename ? pd->errors[i].filename : "???", pd->errors[i].error_position, pd->errors[i].error);
        MOJOSHADER_freePreprocessData(pd);
        return 1;
    }
    if (pd->output && pd->output_len)
        fwrite(pd->output, pd->output_len, 1, stdout);
    printf("\nPRAGMAS:\n");
    int i; for (i = 0; i < pd->pragma_count; i++) {
        const MOJOSHADER_pragma *pr = &pd->pragmas[i];
        printf("%d-%d:%s\n", pr->start, pr->end, pr->septok ? pr->septok : "");
    }
    MOJOSHADER_freePreprocessData(pd);
    return 0;
}

// end of testpragmas.c
