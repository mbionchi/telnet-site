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

void render_separator(struct window *window) {
    scrollok(window->window, 0);
    for (size_t i = 0; i < window->rows; i++) {
        mvwprintw(window->window, i, 0, "%s", i==window->scroll?SEPARATOR_SELECTED:SEPARATOR_REGULAR);
    }
    scrollok(window->window, 1);
}

void render_ncontent(struct window *window) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        size_t cursor_y = 0,
               i = window->scroll;
        while (i < window->content.lines->n_formatted && cursor_y < window->rows) {
            render_nline(window, cursor_y, window->content.lines->formatted[i]);
            if (window->content.lines->formatted[i]->type == ANIM &&
                    window->content.lines->formatted[i]->anim->is_first_line) {
                push_anim_ref_back(window, i);
            }
            i++;
            cursor_y++;
        }
    }
}

void render_nline(struct window *window, size_t cursor_y, struct nline *nline) {
    struct string *s = NULL;
    if (nline->type == TEXT) {
        s = nline->text;
    } else if (nline->type == ANIM) {
        s = nline->anim->frames[nline->anim->current_frame_index].s;
    }
    if (s != NULL) {
        size_t cursor_x = 0; /* assume left-align by default */
        if (nline->align == RIGHT) {
            cursor_x = window->cols - s->len;
        } else if (nline->align == CENTER) {
            cursor_x = (window->cols - s->len)/2;
        }
        scrollok(window->window, 0);
#ifdef ENABLE_WCHAR
        mvwaddwstr(window->window, cursor_y, cursor_x, s->data);
#else
        mvwprintw(window->window, cursor_y, cursor_x, "%s", s->data);
#endif
        scrollok(window->window, 1);
    }
}
