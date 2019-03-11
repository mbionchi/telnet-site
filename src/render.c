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

void render_separator(WINDOW *window, int selected_index) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    scrollok(window, 0);
    for (int i=0; i<maxy; i++) {
        mvwprintw(window, i, 0, "%s", i==selected_index?SEPARATOR_SELECTED:SEPARATOR_REGULAR);
    }
    scrollok(window, 1);
}

// =========================

void render_ncontent(struct window *window) {
    size_t cursor_y = 0,
           i = window->scroll;
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
