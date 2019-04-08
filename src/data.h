/*
 *   This file is part of telnet-site.
 *
 *   telnet-site is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   telnet-site is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with telnet-site.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _TELNET_SITE_DATA_H
#define _TELNET_SITE_DATA_H

#define NO_TRAILING_NEWLINE 0x1

#define ALIGN_HINT ";align"
#define ALIGN_LEFT_HINT "left"
#define ALIGN_RIGHT_HINT "right"
#define ALIGN_CENTER_HINT "center"

#include <stdio.h>
#include <dirent.h>
#include <curses.h>

char *binary_name;

enum content_type { STATIC, DYNAMIC };

enum mode { COMMAND, INSERT };

enum align { LEFT, CENTER, RIGHT };

struct section {
    char *title;
    char *filename;
    enum content_type type;
};

struct string {
    char *data;
    size_t len;
};

struct frame {
    unsigned delay;
    struct string *s;
};

struct animation {
    struct frame *frames;
    size_t current_frame_index;
    size_t n_frames;
    size_t nmemb_frames;
    int loop;
    int is_first_line;
};

struct nline {
    enum { ANIM, TEXT } type;
    enum align align;
    union {
        struct string *text;
        struct animation *anim;
    };
};

struct static_content {
    struct nline **raw;
    size_t n_raw;
    struct nline **formatted;
    size_t n_formatted;
    struct anim_ref *anim_refs;
};

struct dynamic_content {
    void *so_handle;
    void (*init_fun)(WINDOW *window);
    void (*getch_fun)(int ch);
    void (*scroll_fun)(int dy);
    void (*setmode_fun)(enum mode mode);
    void (*kill_fun)();
};

struct content {
    enum content_type type;
    union {
        struct static_content *lines;
        struct dynamic_content *dlib;
    };
};

struct window {
    WINDOW *window;
    size_t cols, rows, scroll;
    struct content content;
};

struct anim_ref {
    size_t index;
    size_t cursor_y;
    size_t counter;
    int running;
    struct anim_ref *next;
};

void gen_err_opening(struct content *content);
int read_content_from_section(struct content *content, struct section *section);

struct nline *string2nline(char *str);
size_t read_nlines(FILE *fp, struct nline ***nlines);
size_t flow_nlines(struct nline **from, size_t n_from, struct nline ***to, int width, int options);
void print_nlines(struct nline **nlines, size_t nmemb);
void free_nlines(struct nline **nlines, size_t nmemb);

size_t gen_index(struct section **sections, size_t nmemb, struct nline ***to, size_t width);

#define ANIM_HINT ";anim"
#define FRAME_HINT ";frame"
#define LOOP_HINT ";loop"
#define NOLOOP_HINT ";noloop"

// ======= old stuff:

void dump_sections(struct section **sections, size_t n_sections);
struct section **read_sections(DIR *dir, char *dirname, size_t *nmemb);
void free_content(struct window *window);

#endif
