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

#include "site.h"

#include "anim.h"
#include "render.h"
#include "scroll.h"
#include "data.h"
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

size_t get_index_width(struct section **sections, size_t n_sections) {
    size_t max_len = 0;
    size_t i;
    for (i=0; i<n_sections; i++) {
        size_t title_len = strlen(sections[i]->title);
        if (title_len > max_len) {
            max_len = title_len;
        }
    }
    return max_len;
}

sig_atomic_t had_winch = 0;
int reload_content = 0;

void winch_handler(int signo) {
    had_winch = 1;
}

/*
 * NOTE:
 *   scrollok(window, 0) needs to be called every time before
 *   doing mvwprintw to avoid forced scroll caused by the
 *   cursor possibly reacing the end of screen, and scrollok(window, 1)
 *   needs to be called after the mvwprintw call.
 * TODO:
 *   - WINCH
 *   - scroll file list
 *   - figure out why scrolling fails over telnet (SOMETIMES????) ?
 *   - error handling everywhere
 */
int site(int argc, char **argv) {
    signal(SIGWINCH, winch_handler);

    struct window index_window;
    struct window separator_window;
    struct window content_window;

    int selected_index = 0;
    int scroll = 0;
    size_t n_sections;
    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        perror("opendir");
        exit(1);
    }
    struct section **sections = read_sections(dir, argv[1], 1, &n_sections);
    closedir(dir);
    FILE *fp = fopen(sections[selected_index]->filename, "r");
    content_window.content.n_raw = read_nlines(fp, &content_window.content.raw);
    fclose(fp);
    initscr();
    cbreak();
    halfdelay(1);
    noecho();
    curs_set(0);
    int index_left_margin = 2;
    int index_cols = get_index_width(sections, n_sections);
    int index_scroll_threshold = 5;

    int separator_cols = strlen(SEPARATOR_SELECTED);
    int separator_rows = LINES;
    int separator_left_margin = index_left_margin + index_cols;

    int content_left_margin = index_left_margin + index_cols + separator_cols;
    int content_right_margin = 8;

    WINDOW *separator_win = newwin(separator_rows, separator_cols, 0, separator_left_margin);

    index_window.cols = get_index_width(sections, n_sections);
    index_window.rows = LINES;
    index_window.scroll = 0;
    index_window.window = newwin(index_window.rows, index_window.cols, 0, index_left_margin);

    index_window.content.anim_refs = NULL;
    index_window.content.n_formatted = gen_index(sections,
                                                 n_sections,
                                                 &index_window.content.formatted,
                                                 index_window.cols);

    content_window.cols = COLS - content_left_margin - content_right_margin;
    content_window.rows = LINES;
    content_window.scroll = 0;
    content_window.window = newwin(content_window.rows, content_window.cols, 0, content_left_margin);

    content_window.content.anim_refs = NULL;
    content_window.content.n_formatted = flow_nlines(content_window.content.raw,
                                                     content_window.content.n_raw,
                                                     &content_window.content.formatted,
                                                     content_window.cols);

    make_scrollable(index_window.window);
    make_scrollable(separator_win);
    make_scrollable(content_window.window);

    render_ncontent(&index_window);
    render_separator(separator_win, selected_index+1);
    render_ncontent(&content_window);

    refresh();
    wrefresh(index_window.window);
    wrefresh(separator_win);
    wrefresh(content_window.window);

    int ch = getch();
    while (ch != 'q') {
        switch(ch) {
            case 'J':
                if (selected_index < n_sections-1) {
                    selected_index++;
                    reload_content = 1;
                    if (selected_index + index_scroll_threshold - index_window.scroll >= index_window.rows &&
                            index_window.scroll + index_window.rows - 1 <= n_sections ) {
                        scroll_ncontent(&index_window, 1);
                    } else {
                        scroll_separator(separator_win, 1);
                    }
                }
                break;
            case 'K':
                if (selected_index > 0) {
                    selected_index--;
                    reload_content = 1;
                    if (selected_index-index_window.scroll-index_scroll_threshold <= 0 &&
                            index_window.scroll > 0) {
                        scroll_ncontent(&index_window, -1);
                    } else {
                        scroll_separator(separator_win, -1);
                    }
                }
                break;
            case 'j':
                scroll_ncontent(&content_window, 1);
                break;
            case 'k':
                scroll_ncontent(&content_window, -1);
                break;
            case 'g':
                if (content_window.scroll > 0) {
                    wclear(content_window.window);
                    content_window.scroll = 0;
                    render_ncontent(&content_window);
                    wrefresh(content_window.window);
                }
                break;
            case 'G':
                if (content_window.content.n_formatted > content_window.rows) {
                    wclear(content_window.window);
                    content_window.scroll = content_window.content.n_formatted-content_window.rows;
                    render_ncontent(&content_window);
                    wrefresh(content_window.window);
                }
                break;
            case ERR:
                anim_tick(&content_window);
                if (had_winch) {
                    free_nlines(content_window.content.formatted, content_window.content.n_formatted);

                    delwin(index_window.window);
                    delwin(separator_win);
                    delwin(content_window.window);
                    endwin();

                    refresh();

                    index_window.rows = LINES;
                    index_window.window = newwin(index_window.rows, index_window.cols, 0, index_left_margin);

                    separator_rows = LINES;
                    separator_win = newwin(separator_rows, separator_cols, 0, separator_left_margin);

                    content_window.cols = COLS - content_left_margin - content_right_margin;
                    content_window.rows = LINES;
                    content_window.scroll = 0;
                    content_window.window = newwin(content_window.rows, content_window.cols, 0, content_left_margin);

                    content_window.content.n_formatted =
                        flow_nlines(content_window.content.raw,
                                    content_window.content.n_raw,
                                    &content_window.content.formatted,
                                    content_window.cols);

                    make_scrollable(index_window.window);
                    make_scrollable(separator_win);
                    make_scrollable(content_window.window);

                    render_ncontent(&index_window);
                    render_separator(separator_win, selected_index+1);
                    render_ncontent(&content_window);

                    refresh();

                    wrefresh(index_window.window);
                    wrefresh(separator_win);
                    wrefresh(content_window.window);

                    had_winch = 0;
                }
                if (reload_content) {
                    free_content(&content_window);
                    free_anim_refs(&content_window);

                    FILE *fp = fopen(sections[selected_index]->filename, "r");
                    content_window.content.n_raw = read_nlines(fp, &content_window.content.raw);
                    fclose(fp);

                    content_window.content.n_formatted =
                        flow_nlines(content_window.content.raw,
                                    content_window.content.n_raw,
                                    &content_window.content.formatted,
                                    content_window.cols);
                    content_window.scroll = 0;

                    wclear(content_window.window);
                    render_ncontent(&content_window);
                    wrefresh(content_window.window);

                    reload_content = 0;
                }
                break;
            default:
                break;
        }
        ch = getch();
    }
    free_content(&content_window);
    free_anim_refs(&content_window);
    delwin(index_window.window);
    delwin(separator_win);
    delwin(content_window.window);
    endwin();
    return 0;
}
