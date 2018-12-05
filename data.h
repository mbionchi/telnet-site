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

struct line {
    char *line;
    struct line *prev, *next;
};

struct section {
    char *title;
    char *filename;
    struct line *content;
};

void print_lines(struct line *lines);
void dump_sections(struct section **sections, size_t n_sections);
struct line *read_lines(FILE *fp);
struct section **read_sections(DIR *dir, char *dirname, int follow_links, size_t *nmemb);
struct line *flow_content(struct line *lines, int width);
void free_lines(struct line *lines);

#endif
