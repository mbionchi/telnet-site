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

#include "scroll.h"

#include "anim.h"
#include "log.h"
#include "site.h"
#include "render.h"
#include "data.h"
#include <string.h>
#include <ncurses.h>

void make_scrollable(WINDOW *window) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    scrollok(window, 1);
    idlok(window, 1);
    wsetscrreg(window, 0, maxy-1);
}

void scroll_separator(struct window *window, int dy) {
    if (0 < dy && window->scroll < window->rows) {
        wscrl(window->window, -1);
        window->scroll++;
        scrollok(window->window, 0);
        mvwprintw(window->window,  0, 0, "%s", SEPARATOR_REGULAR);
        scrollok(window->window, 1);
    } else if (dy < 0 && 1 < window->scroll) {
        wscrl(window->window, 1);
        window->scroll--;
        scrollok(window->window, 0);
        mvwprintw(window->window, window->rows-1, 0, "%s", SEPARATOR_REGULAR);
        scrollok(window->window, 1);
    } else {
        return;
    }
    wrefresh(window->window);
}

void scroll_ncontent(struct window *window, int dy) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        if (dy == 1 && window->scroll+window->rows < window->content.lines->n_formatted) {
            wscrl(window->window, 1);
            window->scroll++;
            render_nline(window->window, window->rows-1, window->content.lines->formatted[window->scroll+window->rows-1]);
            if (window->content.lines->formatted[window->scroll-1]->type == ANIM &&
                    (window->content.lines->formatted[window->scroll]->type != ANIM ||
                     window->content.lines->formatted[window->scroll]->anim->is_first_line)) {
                pop_anim_ref_front(window);
            }
            if (window->content.lines->formatted[window->scroll+window->rows-1]->type == ANIM &&
                    window->content.lines->formatted[window->scroll+window->rows-1]->anim->is_first_line) {
                push_anim_ref_back(window, window->scroll+window->rows-1);
            }
        } else if (dy == -1 && window->scroll > 0) {
            wscrl(window->window, -1);
            window->scroll--;
            render_nline(window->window, 0, window->content.lines->formatted[window->scroll]);
            // was there an animation that went out of focus?
            if (window->content.lines->formatted[window->scroll+window->rows-1]->type == ANIM &&
                    window->content.lines->formatted[window->scroll+window->rows-1]->anim->is_first_line) {
                pop_anim_ref_back(window);
            }
            // is there a new animation that came into focus?
            if (window->content.lines->formatted[window->scroll]->type == ANIM &&
                    (window->content.lines->formatted[window->scroll+1]->type != ANIM ||
                     window->content.lines->formatted[window->scroll+1]->anim->is_first_line)) {
                push_anim_ref_front(window, window->scroll);
            }
        }
        wrefresh(window->window);
    }
}
