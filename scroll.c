#include "scroll.h"
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
