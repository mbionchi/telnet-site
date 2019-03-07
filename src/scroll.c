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

void scroll_content(WINDOW *window, struct line **content_top, struct line **content_bot, int dy) {
    int maxy, maxx;
    getmaxyx(window, maxy, maxx);
    if (dy > 0 && (*content_bot)->next != NULL) {
        wscrl(window, 1);
        *content_top = (*content_top)->next;
        *content_bot = (*content_bot)->next;
        mvwprintw(window, maxy-1, 0, "%s", (*content_bot)->line);
    } else if (dy < 0 && (*content_top)->prev != NULL) {
        wscrl(window, -1);
        *content_top = (*content_top)->prev;
        *content_bot = (*content_bot)->prev;
        mvwprintw(window, 0, 0, "%s", (*content_top)->line);
    } else {
        return;
    }
    wrefresh(window);
}

void top_content(WINDOW *window, struct line **content_top, struct line **content_bot) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    if ((*content_top)->prev != NULL) {
        while ((*content_top)->prev != NULL) {
            *content_top = (*content_top)->prev;
        }
        *content_bot = *content_top;
        for (int i=0; i<maxy-1 && (*content_bot)->next; i++) {
            *content_bot = (*content_bot)->next;
        }
        wclear(window);
        render_content(window, *content_top);
        wrefresh(window);
    }
}

void bot_content(WINDOW *window, struct line **content_top, struct line **content_bot) {
    if ((*content_bot)->next != NULL) {
        while ((*content_bot)->next != NULL) {
            *content_top = (*content_top)->next;
            *content_bot = (*content_bot)->next;
        }
        wclear(window);
        render_content(window, *content_top);
        wrefresh(window);
    }
}

void scroll_index(WINDOW *window, struct section **sections, size_t n_sections, int index_scroll, int dy) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    if (dy > 0) {
        wscrl(window, 1);
        if (maxy+index_scroll-2 < n_sections) {
            size_t title_len = strlen(sections[maxy+index_scroll-2]->title);
            scrollok(window, 0);
            mvwprintw(window, maxy-1, maxx-title_len, "%s", sections[maxy+index_scroll-2]->title);
            scrollok(window, 1);
        }
    } else if (dy < 0) {
        wscrl(window, -1);
        if (index_scroll-1 >= 0) {
            size_t title_len = strlen(sections[index_scroll-1]->title);
            scrollok(window, 0);
            mvwprintw(window, 0, maxx-title_len, "%s", sections[index_scroll-1]->title);
            scrollok(window, 1);
        }
    } else {
        return;
    }
    wrefresh(window);
}

/*
 * this is somewhat horrifying, but the separator is scrolled in the opposite
 * direction from the index!
 */
void scroll_separator(WINDOW *window, int dy) {
    int maxx, maxy;
    getmaxyx(window, maxy, maxx);
    if (dy > 0) {
        wscrl(window, -1);
        mvwprintw(window,  0, 0, "%s", SEPARATOR_REGULAR);
    } else if (dy < 0) {
        wscrl(window, 1);
        scrollok(window, 0);
        mvwprintw(window, maxy-1, 0, "%s", SEPARATOR_REGULAR);
        scrollok(window, 1);
    } else {
        return;
    }
    wrefresh(window);
}

void scroll_ncontent(struct window *window, int dy) {
    if (dy == 1 && window->scroll+window->rows < window->content.n_formatted) {
        wscrl(window->window, 1);
        window->scroll++;
        render_nline(window->window, window->rows-1, window->content.formatted[window->scroll+window->rows-1]);
        if (window->content.formatted[window->scroll-1]->type == ANIM &&
                (window->content.formatted[window->scroll]->type != ANIM ||
                 window->content.formatted[window->scroll]->anim->is_first_line)) {
            pop_anim_ref_front(window);
        }
        if (window->content.formatted[window->scroll+window->rows-1]->type == ANIM &&
                window->content.formatted[window->scroll+window->rows-1]->anim->is_first_line) {
            push_anim_ref_back(window, window->scroll+window->rows-1);
        }
    } else if (dy == -1 && window->scroll > 0) {
        wscrl(window->window, -1);
        window->scroll--;
        render_nline(window->window, 0, window->content.formatted[window->scroll]);
        // was there an animation that went out of focus?
        if (window->content.formatted[window->scroll+window->rows-1]->type == ANIM &&
                window->content.formatted[window->scroll+window->rows-1]->anim->is_first_line) {
            pop_anim_ref_back(window);
        }
        // is there a new animation that came into focus?
        if (window->content.formatted[window->scroll]->type == ANIM &&
                (window->content.formatted[window->scroll+1]->type != ANIM ||
                 window->content.formatted[window->scroll+1]->anim->is_first_line)) {
            push_anim_ref_front(window, window->scroll);
        }
    }
    wrefresh(window->window);
}
