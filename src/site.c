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

struct window index_window;
struct window separator_window;
struct window content_window;

sig_atomic_t had_winch = 0;

void winch_handler(int signo) {
    fprintf(stderr, "winch\n");
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

//=================
/*
    FILE *fp1 = fopen(argv[1], "r");
    struct content c;
    c.n_raw = read_nlines(fp1, &c.raw);
    fclose(fp1);
    c.n_formatted = flow_nlines(c.raw, c.n_raw, &c.formatted, 60);
    print_nlines(c.raw, c.n_raw);
    free_nlines(c.raw, c.n_raw);
    fprintf(stderr, "===========================================\n");
    print_nlines(c.formatted, c.n_formatted);
    exit(0);
*/
//=================

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
//    sections[selected_index]->content = read_lines(fp);
    fclose(fp);
    initscr();
    cbreak();
    halfdelay(1);
    noecho();
    curs_set(0);
    int index_left_margin = 2;
    int index_rows = LINES;
    int index_cols = get_index_width(sections, n_sections);
    int index_scroll = 0;
    int index_scroll_threshold = 5;
    int separator_cols = strlen(SEPARATOR_SELECTED);
    int separator_rows = LINES;
    int separator_left_margin = index_left_margin + index_cols;
    int content_left_margin = index_left_margin + index_cols + separator_cols;
    int content_right_margin = 8;
    WINDOW *index_win = newwin(index_rows, index_cols, 0, index_left_margin);
    WINDOW *separator_win = newwin(separator_rows, separator_cols, 0, separator_left_margin);

    content_window.cols = COLS - content_left_margin - content_right_margin;
    content_window.rows = LINES;
    content_window.scroll = 0;
    content_window.window = newwin(content_window.rows, content_window.cols, 0, content_left_margin);

    content_window.content.anim_refs = NULL;
    content_window.content.n_formatted = flow_nlines(content_window.content.raw,
                                                     content_window.content.n_raw,
                                                     &content_window.content.formatted,
                                                     content_window.cols);

    make_scrollable(index_win);
    make_scrollable(separator_win);
    make_scrollable(content_window.window);
    render_index(index_win, sections, n_sections);
    render_separator(separator_win, selected_index+1);
    render_ncontent(&content_window);
    refresh();
    wrefresh(index_win);
    wrefresh(separator_win);
    wrefresh(content_window.window);

    int ch = getch();
    while (ch != 'q') {
        switch(ch) {
            case 'J':
                if (selected_index < n_sections-1) {
                    if (content_window.content.raw != NULL) {
                        free_nlines(content_window.content.raw, content_window.content.n_raw);
                        content_window.content.raw = NULL;
                    }
                    if (content_window.content.formatted != NULL) {
                        free_nlines(content_window.content.formatted, content_window.content.n_formatted);
                        content_window.content.formatted = NULL;
                    }
                    selected_index++;
                    if (content_window.content.raw == NULL) {
                        FILE *fp = fopen(sections[selected_index]->filename, "r");
                        content_window.content.n_raw = read_nlines(fp, &content_window.content.raw);
                        fclose(fp);
                    }
                    if (content_window.content.formatted == NULL) {
                        content_window.content.n_formatted = flow_nlines(content_window.content.raw,
                                                                         content_window.content.n_raw,
                                                                         &content_window.content.formatted,
                                                                         content_window.cols);
                    }
                    wclear(content_window.window);
                    render_ncontent(&content_window);

                    if (selected_index + index_scroll_threshold - index_scroll >= index_rows &&
                            index_scroll + index_rows - 1 <= n_sections ) {
                        index_scroll++;
                        scroll_index(index_win, sections, n_sections, index_scroll, 1);
                    } else {
                        scroll_separator(separator_win, 1);
                    }

                    refresh();
                    wrefresh(content_window.window);
                }
                break;

/*
            case 'K':
                if (selected_index > 0) {
                    if (sections[selected_index]->content != NULL) {
                        free_lines(sections[selected_index]->content);
                        sections[selected_index]->content = NULL;
                    }
                    selected_index--;
                    if (sections[selected_index]->content == NULL) {
                        FILE *fp = fopen(sections[selected_index]->filename, "r");
                        sections[selected_index]->content = read_lines(fp);
                        fclose(fp);
                    }
                    wclear(content_win);
                    free_lines(content_head);
                    content_head = flow_content(sections[selected_index]->content, content_cols);
                    content_top = content_head;
                    content_bot = content_head;
                    for (int i=0; i<content_rows-1 && content_bot->next != NULL; i++) {
                        content_bot = content_bot->next;
                    }

                    if (selected_index-index_scroll-index_scroll_threshold <= 0 &&
                            index_scroll > 0) {
                        index_scroll--;
                        scroll_index(index_win, sections, n_sections, index_scroll, -1);
                    } else {
                        scroll_separator(separator_win, -1);
                    }

                    render_content(content_win, content_head);
                    refresh();
                    wrefresh(content_win);
                }
                break;
*/

            case 'j':
                scroll_ncontent(&content_window, 1);
                break;
            case 'k':
                scroll_ncontent(&content_window, -1);
                break;

/*
            case 'g':
                top_content(content_win, &content_top, &content_bot);
                break;
            case 'G':
                bot_content(content_win, &content_top, &content_bot);
                break;
*/

            case ERR:
                anim_tick(&content_window);
                /*
                 * check for:
                 *   - animation updates
                 *   - window redraw
                 */
                break;
            default:
                break;
        }
        ch = getch();
    }
    delwin(content_window.window);
    delwin(index_win);
    delwin(separator_win);
    endwin();
    return 0;
}
