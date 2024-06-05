#ifndef _SIMPLE_ARRAY_H
#define _SIMPLE_ARRAY_H
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * lifetime: based input src and result allocated.
 */
struct simp_arr {
    // value string pointer
    char *key;
    struct simp_arr *sub,       // extra value
                    *next,      // list next value
                    *parent;    // parent (invert .value, is not .next)
};

enum simp_arr_parse_stat {
    SIMPARR_STAT_INIT = 0,
    SIMPARR_STAT_NORMAL,
    SIMPARR_STAT_COMMMENT,
    SIMPARR_STAT_PRE_LF,
    SIMPARR_STAT_KEY,
    SIMPARR_STAT_HARD_STRING,
    SIMPARR_STAT_ALLOC_ARR,
    SIMPARR_STAT_ALLOCATED_ARR,
    SIMPARR_STAT_SUB_ARR_INITED,
    SIMPARR_STAT_STRING,
    SIMPARR_STAT_STRING_ESCAPE,
    SIMPARR_STAT_STRING_ESCAPE_HEXCH_B1,
    SIMPARR_STAT_STRING_ESCAPE_HEXCH_B2,
    SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_HIGH,
    SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_LOW,
    SIMPARR_STAT_STRING_COMMENT,
    SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT,
    SIMPARR_STAT_STRING_ERR_NEVER_CLOSED_ARR,
    SIMPARR_STAT_STRING_ERR_UNEXPECTED_ARR_CLOSE,
};
char *simp_arr_stat_debug(enum simp_arr_parse_stat stat) {
    switch (stat) {
    case SIMPARR_STAT_INIT: return "INIT";
    case SIMPARR_STAT_NORMAL: return "NORMAL";
    case SIMPARR_STAT_COMMMENT: return "COMMMENT";
    case SIMPARR_STAT_PRE_LF: return "PRE_LF";
    case SIMPARR_STAT_KEY: return "KEY";
    case SIMPARR_STAT_HARD_STRING: return "HARD_STRING";
    case SIMPARR_STAT_ALLOC_ARR: return "ALLOC_ARR";
    case SIMPARR_STAT_ALLOCATED_ARR: return "ALLOCATED_ARR";
    case SIMPARR_STAT_SUB_ARR_INITED: return "SUB_ARR_INITED";
    case SIMPARR_STAT_STRING: return "STRING";
    case SIMPARR_STAT_STRING_ESCAPE: return "STRING_ESCAPE";
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B1: return "STRING_ESCAPE_HEXCH_B1";
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B2: return "STRING_ESCAPE_HEXCH_B2";
    case SIMPARR_STAT_STRING_COMMENT: return "STRING_COMMENT";
    case SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT: return "STRING_ESCAPE_NL_IGNORE_NEXT";
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_HIGH: return "STRING_ESCAPE_HEXCH_BN_HIGH";
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_LOW: return "STRING_ESCAPE_HEXCH_BN_LOW";
    case SIMPARR_STAT_STRING_ERR_NEVER_CLOSED_ARR: return "STRING_ERR_NEVER_CLOSED_ARR";
    case SIMPARR_STAT_STRING_ERR_UNEXPECTED_ARR_CLOSE: return "STRING_ERR_UNEXPECTED_ARR_CLOSE";
    }
    assert(false);
}
struct simp_arr_parse_meta {
    char *src, // input data cursor, char and char* need mutable
         *str_edit,
         inplace_ch;
    struct simp_arr head,
                    *cur,
                    *last_alloc;
    size_t init_input_src_addr; // 记录最初的输入字符串地址, 用于错误回溯
    enum simp_arr_parse_stat stat,
                             prev_stat;
    bool is_allocated;
};

void simp_arr_init_node(struct simp_arr *node) {
    node->key = NULL;
    node->sub = NULL;
    node->parent = NULL;
    node->next = NULL;
}
void simp_arr_make_next_node(
        struct simp_arr *node,
        struct simp_arr *next)
{
    simp_arr_init_node(next);

    node->next = next;
    next->parent = node->parent;
}
void simp_arr_make_sub_node(
        struct simp_arr *node,
        struct simp_arr *sub)
{
    simp_arr_init_node(sub);

    node->sub = sub;
    sub->parent = node;
}

int simp_arr_fmt_string(char *s, void (*f)(char *));

/*
 * return alloc size, 0 is finished.
 */
size_t simp_arr_parse(struct simp_arr_parse_meta *meta) {
    char ch, escaped_ch;
    for (;; meta->inplace_ch = *++meta->src) {
inplace:
        ch = meta->inplace_ch;

        switch (meta->stat) {
        case SIMPARR_STAT_INIT:
            meta->stat = SIMPARR_STAT_NORMAL;
            meta->inplace_ch = *meta->src;

            simp_arr_init_node(&meta->head);

            meta->cur = &meta->head;
            meta->is_allocated = false;

            meta->init_input_src_addr = (size_t)meta->src;

            goto inplace;

        case SIMPARR_STAT_NORMAL:
            switch (ch) {
            case ' ':
            case '\t':
            case '\n':
                break;

            case '\r':
                meta->prev_stat = SIMPARR_STAT_NORMAL;
                meta->stat = SIMPARR_STAT_PRE_LF;
                break;

            case '\0':
                goto ed;
                break;

            case ';':
                meta->stat = SIMPARR_STAT_COMMMENT;
                break;

            case '{':
                if (!meta->is_allocated && meta->cur->sub != NULL) {
                    // 已经存在子节点, 分配一个next
                    meta->prev_stat = SIMPARR_STAT_NORMAL;
                    meta->stat = SIMPARR_STAT_ALLOC_ARR;
                    goto inplace;
                }
                if (meta->is_allocated) {
                    simp_arr_make_next_node(meta->cur, meta->last_alloc);
                    meta->cur = meta->cur->next;
                    meta->is_allocated = false;
                }
                meta->stat = SIMPARR_STAT_SUB_ARR_INITED;
                break;

            case '}':
                if (meta->cur->parent == NULL) {
                    meta->stat = SIMPARR_STAT_STRING_ERR_UNEXPECTED_ARR_CLOSE;
                    return 0;
                }
                meta->cur = meta->cur->parent;
                break;

            case '"':
            case '\'':
            default:
                if (!meta->is_allocated
                        && (meta->cur->key != NULL
                            || meta->cur->sub != NULL))
                {
                    meta->prev_stat = SIMPARR_STAT_NORMAL;
                    meta->stat = SIMPARR_STAT_ALLOC_ARR;
                    goto inplace;
                }
                if (meta->is_allocated) {
                    assert(meta->cur != meta->last_alloc);
                    simp_arr_make_next_node(meta->cur, meta->last_alloc);
                    meta->cur = meta->cur->next;
                    meta->is_allocated = false;
                }
                switch (ch) {
                case '"':
                    meta->stat = SIMPARR_STAT_STRING;
                    meta->cur->key = meta->src + 1;
                    meta->str_edit = meta->cur->key;
                    break;

                case '\'':
                    meta->stat = SIMPARR_STAT_HARD_STRING;
                    meta->cur->key = meta->src + 1;
                    break;

                default:
                    meta->stat = SIMPARR_STAT_KEY;
                    meta->cur->key = meta->src;
                    break;
                }
                break;
            }
            break;

        case SIMPARR_STAT_COMMMENT:
            switch (ch) {
            case '\0':
                meta->stat = SIMPARR_STAT_NORMAL;
                return 0;

            case '\n':
                meta->stat = SIMPARR_STAT_NORMAL;
                break;

            case '\r':
                meta->prev_stat = SIMPARR_STAT_NORMAL;
                meta->stat = SIMPARR_STAT_PRE_LF;
                break;
            }
            break;

        case SIMPARR_STAT_PRE_LF:
            if (ch == '\n') {
                meta->stat = meta->prev_stat;
                break;
            }
            return 0;

        case SIMPARR_STAT_ALLOC_ARR:
            meta->stat = SIMPARR_STAT_ALLOCATED_ARR;
            return sizeof(struct simp_arr);

        case SIMPARR_STAT_ALLOCATED_ARR:
            meta->stat = meta->prev_stat;
            meta->is_allocated = true;
            goto inplace;

        case SIMPARR_STAT_HARD_STRING:
            switch (ch) {
            case '\r':
            case '\n':
            case '\0':
                return 0;

            case '\'':
                *meta->src = '\0';
                meta->stat = SIMPARR_STAT_NORMAL;
                break;
            }
            break;

        case SIMPARR_STAT_SUB_ARR_INITED:
            if (!meta->is_allocated) {
                meta->prev_stat = SIMPARR_STAT_SUB_ARR_INITED;
                meta->stat = SIMPARR_STAT_ALLOC_ARR;
                goto inplace;
            }
            meta->is_allocated = false;
            simp_arr_make_sub_node(meta->cur, meta->last_alloc);
            meta->cur = meta->cur->sub;
            meta->stat = SIMPARR_STAT_NORMAL;
            goto inplace;

        case SIMPARR_STAT_KEY:
            switch (ch) {
            case '\0':
            case '"':
            case '\'':
            case '{':
            case '}':
            case ';':
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                *meta->src = '\0';
                meta->stat = SIMPARR_STAT_NORMAL;
                goto inplace;
            }
            break;

        case SIMPARR_STAT_STRING:
            switch (ch) {
            case '\r':
            case '\n':
            case '\0':
                return 0;

            case '"':
                *meta->str_edit = '\0';
                meta->stat = SIMPARR_STAT_NORMAL;
                break;

            case '\\':
                meta->stat = SIMPARR_STAT_STRING_ESCAPE;
                break;

            default:
                *meta->str_edit++ = *meta->src;
                break;
            }
            break;

        case SIMPARR_STAT_STRING_ESCAPE:
            switch (ch) {
            case '\\':
            case '"':
                *meta->str_edit++ = ch;
                meta->stat = SIMPARR_STAT_STRING;
                break;

            case 'n':
                *meta->str_edit++ = '\n';
                meta->stat = SIMPARR_STAT_STRING;
                break;

            case 'r':
                *meta->str_edit++ = '\r';
                meta->stat = SIMPARR_STAT_STRING;
                break;

            case 't':
                *meta->str_edit++ = '\t';
                meta->stat = SIMPARR_STAT_STRING;
                break;

            case 'a':
                *meta->str_edit++ = '\a';
                meta->stat = SIMPARR_STAT_STRING;
                break;

            case 'e':
                *meta->str_edit++ = '\033';
                meta->stat = SIMPARR_STAT_STRING;
                break;

            case ';':
                meta->stat = SIMPARR_STAT_STRING_COMMENT;
                break;

            case '\n':
                meta->stat = SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT;
                break;

            case '\r':
                meta->prev_stat = SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT;
                meta->stat = SIMPARR_STAT_PRE_LF;
                break;

            case 'x':
                meta->stat = SIMPARR_STAT_STRING_ESCAPE_HEXCH_B1;
                break;

            default:
                return 0;
            }
            break;

        case SIMPARR_STAT_STRING_COMMENT:
            switch (ch) {
            case '\0':
                return 0;

            case '\n':
                meta->stat = SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT;
                break;

            case '\r':
                meta->prev_stat = SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT;
                meta->stat = SIMPARR_STAT_PRE_LF;
                break;
            }
            break;

        case SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT:
            if (ch != ' ' && ch != '\t') {
                meta->stat = SIMPARR_STAT_STRING;
                goto inplace;
            }
            break;

        case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B1:
            if (ch == '{') {
                meta->stat = SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_HIGH;
                break;
            }
        case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B2:
            if (ch == '_')
                break;
            if (! ((ch >= '0' && ch <= '9')
                        || (ch >= 'a' && ch <= 'f')
                        || (ch >= 'A' && ch <= 'F')))
                return 0;

            escaped_ch = ch >= '0' && ch <= '9'
                ? ch - '0'
                : ch >= 'a' && ch <= 'f'
                    ? ch - 'a' + 10
                    : ch >= 'A' && ch <= 'F'
                        ? ch - 'A' + 10
                        : (assert(false), 0);

            switch (meta->stat) {
            case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B1:
                *meta->str_edit = escaped_ch << 4;
                meta->stat = SIMPARR_STAT_STRING_ESCAPE_HEXCH_B2;
                break;

            case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B2:
                *meta->str_edit++ += escaped_ch;
                meta->stat = SIMPARR_STAT_STRING;
                break;

            default: assert(false);
            }
            break;

        case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_LOW:
            if (ch == '}')
                return 0;
            if (ch == '_')
                return 0;
        case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_HIGH:
            if (ch == '}') {
                meta->stat = SIMPARR_STAT_STRING;
                break;
            }
            if (ch == '_')
                break;

            if (! ((ch >= '0' && ch <= '9')
                        || (ch >= 'a' && ch <= 'f')
                        || (ch >= 'A' && ch <= 'F')))
                return 0;

            escaped_ch = ch >= '0' && ch <= '9'
                ? ch - '0'
                : ch >= 'a' && ch <= 'f'
                    ? ch - 'a' + 10
                    : ch >= 'A' && ch <= 'F'
                        ? ch - 'A' + 10
                        : (assert(false), 0);

            switch (meta->stat) {
            case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_HIGH:
                *meta->str_edit = escaped_ch << 4;
                meta->stat = SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_LOW;
                break;

            case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_LOW:
                *meta->str_edit++ += escaped_ch;
                meta->stat = SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_HIGH;
                break;

            default: assert(false);
            }
            break;

        case SIMPARR_STAT_STRING_ERR_NEVER_CLOSED_ARR:
        case SIMPARR_STAT_STRING_ERR_UNEXPECTED_ARR_CLOSE:
            return 0;
        }
    }
ed:

    if (meta->cur->parent != NULL)
        meta->stat = SIMPARR_STAT_STRING_ERR_NEVER_CLOSED_ARR;
    return 0;
}

// - Returning 0 is no escape, e.g key
// - Returning 1 is hard string, e.g 'foo bar'
// - Returning 2 is string, e.g "abc\ndef"
// - Returning -1 is null ptr
int simp_arr_string_level(char *s) {
    if (s == NULL) return -1;
    if (*s == '\0') return 1;
    signed char level = 0;
    for (char ch; (ch = *s++) != '\0';) {
        switch (ch) {
        case ' ': case '{': case '}': case '"':
        case ';':
            level = 1;
        }
        if (ch <= '\x1f' || ch == '\'' || ch == '\x7f') {
            level = 2;
            break;
        }
    }
    return level;
}

int simp_arr_fmt_string(char *s, void (*f)(char *)) {
    int str_level = simp_arr_string_level(s);
    if (str_level == -1) return str_level;
    switch (str_level) {
    case 0:
        f(s);
        return 0;
    case 1:
        f("'");
        f(s);
        f("'");
        return 0;
    case 2:
    {
        f("\"");

        for (char ch; (ch = *s++) != '\0';) {
            switch (ch) {
            case '\n':      f("\\n");   break;
            case '\r':      f("\\r");   break;
            case '\t':      f("\\t");   break;
            case '\a':      f("\\a");   break;
            case '\x1b':    f("\\e");   break;
            case '"':       f("\\\"");  break;
            case '\\':      f("\\\\");  break;
            case '\x7f':    f("\\x7f"); break;
            default:
                if (ch <= '\x1f' && ch > '\x00') {
                    f("\\x");
                    int i = 8;
                    do {
                        i -= 4;
                        switch ((ch >> i) & 0xf) {
                        case 0:  f("0"); break;
                        case 1:  f("1"); break;
                        case 2:  f("2"); break;
                        case 3:  f("3"); break;
                        case 4:  f("4"); break;
                        case 5:  f("5"); break;
                        case 6:  f("6"); break;
                        case 7:  f("7"); break;
                        case 8:  f("8"); break;
                        case 9:  f("9"); break;
                        case 10: f("a"); break;
                        case 11: f("b"); break;
                        case 12: f("c"); break;
                        case 13: f("d"); break;
                        case 14: f("e"); break;
                        case 15: f("f"); break;
                        }
                    } while (i != 0);
                } else {
                    char cbuf[2] = { ch, '\0' };
                    f(cbuf);
                }
            }
        }

        f("\"");
        return 0;
    } break;
    }
    return -2;
}

void simp_arr_fmt(
        struct simp_arr *node,
        char *indent,
        int indent_level,
        void (*f)(char *))
{
    do {
        for (int i = 0; i < indent_level; i++) f(indent);
        if (node->key != NULL) {
            simp_arr_fmt_string(node->key, f);
            if (node->sub != NULL)
                f(" ");
        }
        if (node->sub != NULL) {
            if (node->sub->key == NULL
                    && node->sub->next == NULL
                    && node->sub->sub == NULL)
            {
                f("{}");
            } else if (node->sub->key != NULL
                    && node->sub->next == NULL
                    && node->sub->sub == NULL)
            {
                f("{ ");
                simp_arr_fmt_string(node->sub->key, f);
                f(" }");
            } else {
                f("{\n");
                simp_arr_fmt(node->sub, indent, indent_level+1, f);
                for (int i = 0; i < indent_level; i++)
                    f(indent);
                f("}");
            }
        }
        f("\n");
    } while ((node = node->next) != NULL);
}

void simp_arr_fmt_short(
        struct simp_arr *node,
        void (*f)(char *))
{
    struct simp_arr *prev = NULL;
    do {
        if (prev != NULL && prev->sub == NULL)
            f(" ");
        if (node->key != NULL)
            simp_arr_fmt_string(node->key, f);
        if (node->sub != NULL) {
            f("{");
            simp_arr_fmt_short(node->sub, f);
            f("}");
        }
    } while ((node = (prev = node)->next) != NULL);
}

int simp_arr_fmt_result(
        struct simp_arr_parse_meta *meta,
        void (*f)(char *))
{
    if (meta->stat == SIMPARR_STAT_NORMAL
            || meta->stat == SIMPARR_STAT_INIT)
        return 1;

    char *msg;

    switch (meta->stat) {
    case SIMPARR_STAT_PRE_LF:                           msg = "expected LF (\\n)";                  break;
    case SIMPARR_STAT_HARD_STRING:                      msg = "expected single quote (')";          break;
    case SIMPARR_STAT_SUB_ARR_INITED:                   msg = "expected LF (\\n)";                  break;
    case SIMPARR_STAT_STRING:                           msg = "expected double quote (')";          break;
    case SIMPARR_STAT_STRING_ESCAPE:                    msg = "expected string escape";             break;
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B1:           msg = "expected hex digit";                 break;
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_B2:           msg = "expected hex digit";                 break;
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_HIGH:      msg = "expected hex digit";                 break;
    case SIMPARR_STAT_STRING_ESCAPE_HEXCH_BN_LOW:       msg = "expected hex digit";                 break;
    case SIMPARR_STAT_STRING_COMMENT:                   msg = "expected newline (\\n or \\r\\n)";   break;
    case SIMPARR_STAT_STRING_ERR_NEVER_CLOSED_ARR:      msg = "expected array close (})";           break;
    case SIMPARR_STAT_STRING_ERR_UNEXPECTED_ARR_CLOSE:  msg = "unexpected array close (})";         break;

    case SIMPARR_STAT_KEY:
    case SIMPARR_STAT_COMMMENT:
    case SIMPARR_STAT_ALLOC_ARR:
    case SIMPARR_STAT_ALLOCATED_ARR:
    case SIMPARR_STAT_STRING_ESCAPE_NL_IGNORE_NEXT:
        assert(("invalid exit status", false));

    case SIMPARR_STAT_INIT:
    case SIMPARR_STAT_NORMAL:
        assert(false);
    }

    size_t index = (size_t)meta->src - meta->init_input_src_addr;
    int digit = 1;
    for (int n = index / 10; n > 0; n /= 10, digit++) {}
    f("from index ");
    for (int i = 0; i < digit; i++) {
        int dn = 1;
        for (int j = 0; j++ < (digit - i - 1);)
            dn *= 10;
        switch (index / dn % 10) {
        case 0: f("0"); break;
        case 1: f("1"); break;
        case 2: f("2"); break;
        case 3: f("3"); break;
        case 4: f("4"); break;
        case 5: f("5"); break;
        case 6: f("6"); break;
        case 7: f("7"); break;
        case 8: f("8"); break;
        case 9: f("9"); break;
        }
    }

    f(", ");

    f(msg);
    return 0;
}
#endif
