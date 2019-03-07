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

#include "anim.h"
#include "log.h"
#include "data.h"
#include "site.h"
#include <string.h>
#include <ncurses.h>

void render_index(WINDOW *window, struct section **sections, size_t n_sections) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    scrollok(window, 0);
    for (size_t i=0; i<n_sections && i<maxy-1; i++) {
        size_t title_len = strlen(sections[i]->title);
        mvwprintw(window, i+1, maxx-title_len, "%s", sections[i]->title);
    }
    scrollok(window, 1);
}

void render_separator(WINDOW *window, int selected_index) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    scrollok(window, 0);
    for (int i=0; i<maxy; i++) {
        mvwprintw(window, i, 0, "%s", i==selected_index?SEPARATOR_SELECTED:SEPARATOR_REGULAR);
    }
    scrollok(window, 1);
}

void render_content(WINDOW *window, struct line *lines) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    scrollok(window, 0);
    int cursor_y = 0;
    struct line *iter = lines;
    while (iter && cursor_y < maxy) {
        mvwprintw(window, cursor_y, 0, "%s", iter->line);
        cursor_y++;
        iter = iter->next;
    }
    scrollok(window, 1);
}

// =========================

void render_ncontent(struct window *window) {
    size_t cursor_y = 0,
           i = 0;
    while (i < window->content.n_formatted && cursor_y < window->rows) {
        render_nline(window->window, cursor_y, window->content.formatted[i]);
        if (window->content.formatted[i]->type == ANIM &&
                window->content.formatted[i]->anim->is_first_line) {
            push_anim_ref_back(window, i);
        }
        i++;
        cursor_y++;
    }
}

void render_nline(WINDOW *window, size_t cursor_y, struct nline *nline) {
    scrollok(window, 0);
    if (nline->type == TEXT) {
        mvwprintw(window, cursor_y, 0, "%s", nline->text->data);
    } else if (nline->type == ANIM) {
        mvwprintw(window, cursor_y, 0, nline->anim->frames[nline->anim->current_frame_index].s->data);
    }
    scrollok(window, 1);
}
