#include "../simple_array.h"
#include <alloca.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_handle(char *s) {
    printf("%s", s);
}
void eprint_handle(char *s) {
    fprintf(stderr, "%s", s);
}

int main(int argc, char *argv[]) {
    size_t size = 32, i = 0;
    char *s = (char*)malloc(sizeof(char) * size);
    int exit_code = 0;

    if (argc == 1) {
        for (;;) {
            int ch = getchar();
            if (ch < 0)
                break;
            if (i+2 >= size)
                s = (char*)realloc(s, sizeof(char) * (size <<= 1));
            s[i++] = ch;
        }
        s[i] = '\0';
    } else {
        fprintf(stderr, "不需要参数");
        return 2;
    }

    struct simp_arr_parse_meta
        meta = {
            .src = s,
            .stat = SIMPARR_STAT_INIT,
        };

    size_t alloc_size;

    while ((alloc_size = simp_arr_parse(&meta)))
        meta.last_alloc = (struct simp_arr*)alloca(alloc_size);

    if (simp_arr_fmt_result(&meta, eprint_handle)) {
        simp_arr_fmt_short(&meta.head, print_handle);
        fputc('\0', stdout);
        simp_arr_fmt(&meta.head, "  ", 0, print_handle);
        fputc('\0', stdout);
        fflush(stdout);
    } else {
        exit_code = 2;
    }

    free(s);
    return exit_code;
}
