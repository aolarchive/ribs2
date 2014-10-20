/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "json.h"

inline void json_stack_item_set(struct json_stack_item *si, char *b, char *e)
{
    si->begin = b;
    si->end = e;
}

inline void json_stack_item_reset(struct json_stack_item *si)
{
    si->begin = si->end = NULL;
}

inline int json_stack_item_isset(struct json_stack_item *si)
{
    return si->begin != si->end;
}

static void null_callback(struct json *js, char *kb, char *ke, char *vb, char *ve)
{
    UNUSED(js);
    UNUSED(kb);
    UNUSED(ke);
    UNUSED(vb);
    UNUSED(ve);
}

static void null_block_callback(struct json *js, char *kb, char *ke)
{
    UNUSED(js);
    UNUSED(kb);
    UNUSED(ke);
}

inline void json_reset_callbacks(struct json *js)
{
    js->callback_string = &null_callback;
    js->callback_primitive = &null_callback;
    js->callback_block_begin = &null_block_callback;
    js->callback_block_end = &null_block_callback;
}

inline int json_init(struct json *js)
{
    json_reset_callbacks(js);
    if (0 > vmbuf_init(&js->stack, 4096)) {
        js->err = "init";
        return -1;
    }
    return 0;
}

int json_parse_string(struct json *js)
{
    ++js->cur; // skip the starting \"
    char *start = js->cur;
    for (; *js->cur; ++js->cur) {
        if ('\"' == *js->cur) {
            break;
        } else if ('\\' == *js->cur) {
            ++js->cur;
            if (0 == *js->cur) // error
                return -1;
        }
    }
    if ('\"' != *js->cur) {
        js->err = "string";
        return -1; // error
    }
    json_stack_item_set(&js->last_string, start, js->cur);

    (*js->callback_string)(js, js->last_key.begin, js->last_key.end, start, js->cur);
    ++js->cur; // skip the ending \"
    return 0;
}

int json_parse_primitive(struct json *js)
{
    char *start = js->cur;
    for (; *js->cur; ++js->cur)
    {
        switch (*js->cur)
        {
        case ']':
        case '}':
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
            (*js->callback_primitive)(js, js->last_key.begin, js->last_key.end, start, js->cur);
            json_stack_item_set(&js->last_string, start, js->cur);
            return 0;
        }
    }
    js->err = "primitive";
    return -1;
}

inline int json_parse(struct json *js, char *str)
{
    json_stack_item_reset(&js->last_key);
    js->level = 0;
    js->cur = str;
    json_stack_item_reset(&js->last_string);
    for (; *js->cur; )
    {
        switch(*js->cur)
        {
        case '{':
        case '[':
            (*js->callback_block_begin)(js, js->last_string.begin, js->last_string.end);
            json_stack_item_reset(&js->last_key);
            vmbuf_memcpy(&js->stack, &js->last_string, sizeof(struct json_stack_item));
            json_stack_item_reset(&js->last_string);
            ++js->level;
            break;
        case '}':
        case ']':
            if (js->level == 0) {
                js->err = "unbalanced parenthesis";
                return -1;
            }
            json_stack_item_reset(&js->last_key);
            vmbuf_wrewind(&js->stack, sizeof(struct json_stack_item));
            js->last_string = *(struct json_stack_item *)vmbuf_wloc(&js->stack);
            (*js->callback_block_end)(js, js->last_string.begin, js->last_string.end);
            --js->level;
            break;
        case '\"':
            if (0 > json_parse_string(js))
                return -1;
            continue;
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 't':
        case 'f':
        case 'n':
            if (0 > json_parse_primitive(js))
                return -1;
            continue;
        case ':':
            js->last_key = js->last_string;
            // no break here
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ',':
            json_stack_item_reset(&js->last_key);
            break;
        default:
            break;
        }
        ++js->cur;
    }
    return 0;
}

void json_unescape_str(char *buf)
{
    char table[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
        0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
        0x60, 0x61, 0x08, 0x63, 0x64, 0x65, 0x0C, 0x67,
        0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x0A, 0x6F,
        0x70, 0x71, 0x0D, 0x73, 0x09, 0x75, 0x0B, 0x77,
        0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
        0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
        0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
        0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
        0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
        0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
        0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
        0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
        0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
    };
    char *in = buf, *out = buf;
    for (; *in; )
    {
        switch (*in)
        {
        case '\\':
            ++in;
            *out++ = table[(uint8_t)(*in++)];
            break;
        default:
            *out++ = *in++;
        }
    }
    *out=0;
}

#define ESC_SEQ(c) *d++='\\';*d++=c;break;
size_t json_escape_str(char *d, const char *s) {
    char *d0 = d;
    for(;*s;++s){
        switch(*s){
        case '"':
        case '\\':
        case '/':ESC_SEQ(*s);
        case '\b':ESC_SEQ('b');
        case '\f':ESC_SEQ('f');
        case '\n':ESC_SEQ('n');
        case '\r':ESC_SEQ('r');
        case '\t':ESC_SEQ('t');
        default: *d++ = *s;
        }
    }
    *d = 0;
    return d - d0;
}

size_t json_escape_str_vmb(struct vmbuf *buf, const char *s) {
    vmbuf_resize_if_less(buf, strlen(s) * 2 + 1);
    size_t l = json_escape_str(vmbuf_wloc(buf), s);
    vmbuf_wseek(buf, l);
    return l;
}

char json_copy_key(const char *kb, const char *ke, char *strOut, size_t strOutLen) {
    size_t len = ke - kb;
    if((len + 1) > strOutLen) return -1;
    memcpy(strOut, kb, len);
    strOut[len] = 0;
    return 0;
}
