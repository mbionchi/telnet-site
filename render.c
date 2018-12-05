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
