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

#include <stdio.h>
#include <dirent.h>
#include <curses.h>

char *binary_name;

struct line {
    char *line;
    struct line *prev, *next;
};

struct section {
    char *title;
    char *filename;
    struct line *content;
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
    union {
        struct string *text;
        struct animation *anim;
    };
};

struct content {
    struct nline **raw;
    size_t n_raw;
    struct nline **formatted;
    size_t n_formatted;
    struct anim_ref *anim_refs;
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

size_t gen_err_opening(struct nline ***nlines);

size_t read_nlines(FILE *fp, struct nline ***nlines);
size_t flow_nlines(struct nline **from, size_t n_from, struct nline ***to, int width);
void print_nlines(struct nline **nlines, size_t nmemb);
void free_nlines(struct nline **nlines, size_t nmemb);

size_t gen_index(struct section **sections, size_t nmemb, struct nline ***to, size_t width);

#define ANIM_HINT ";anim"
#define FRAME_HINT ";frame"
#define LOOP_HINT ";loop"
#define NOLOOP_HINT ";noloop"

// ======= old stuff:

void print_lines(struct line *lines);
void dump_sections(struct section **sections, size_t n_sections);
struct section **read_sections(DIR *dir, char *dirname, int follow_links, size_t *nmemb);
struct line *flow_content(struct line *lines, int width);
void free_lines(struct line *lines);
void free_content(struct window *window);

#endif
