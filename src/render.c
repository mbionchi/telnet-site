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

#include "render.h"
#include "data.h"
#include <string.h>
#include <ncurses.h>

void render_index(WINDOW *window, struct section **sections, size_t n_sections) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    for (size_t i=0; i<n_sections && i<maxy-1; i++) {
        size_t title_len = strlen(sections[i]->title);
        mvwprintw(window, i+1, maxx-title_len, "%s", sections[i]->title);
    }
}

void render_separator(WINDOW *window, int selected_index) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    for (int i=0; i<maxy; i++) {
        mvwprintw(window, i, 0, "%s", i==selected_index?SEPARATOR_SELECTED:SEPARATOR_REGULAR);
    }
}

void render_content(WINDOW *window, struct line *lines) {
    int cursor_y = 0;
    struct line *iter = lines;
    while (iter) {
        mvwprintw(window, cursor_y, 0, "%s", iter->line);
        cursor_y++;
        iter = iter->next;
    }
}
