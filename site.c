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
#include "scroll.h"
#include "data.h"
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

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
int main(int argc, char **argv) {
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
    sections[selected_index]->content = read_lines(fp);
    fclose(fp);
    initscr();
    raw();
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
    int content_rows = LINES;
    int content_cols = COLS - content_left_margin - content_right_margin;
    WINDOW *index_win = newwin(index_rows, index_cols, 0, index_left_margin);
    WINDOW *separator_win = newwin(separator_rows, separator_cols, 0, separator_left_margin);
    WINDOW *content_win = newwin(content_rows, content_cols, 0, content_left_margin);
    struct line *content_head = flow_content(sections[selected_index]->content, content_cols);
    struct line *content_top = content_head;
    struct line *content_bot = content_head;
    for (int i=0; i<content_rows-1 && content_bot->next != NULL; i++) {
        content_bot = content_bot->next;
    }
    render_index(index_win, sections, n_sections);
    render_separator(separator_win, selected_index+1);
    render_content(content_win, content_head);
    make_scrollable(index_win);
    make_scrollable(separator_win);
    make_scrollable(content_win);
    refresh();
    wrefresh(index_win);
    wrefresh(separator_win);
    wrefresh(content_win);
    int ch = getch();
    while (ch != 'q') {
        switch(ch) {
            case 'J':
                if (selected_index < n_sections-1) {
                    if (sections[selected_index]->content != NULL) {
                        free_lines(sections[selected_index]->content);
                        sections[selected_index]->content = NULL;
                    }
                    selected_index++;
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
                    render_content(content_win, content_head);

                    if (selected_index + index_scroll_threshold - index_scroll >= index_rows &&
                            index_scroll + index_rows - 1 <= n_sections ) {
                        index_scroll++;
                        scroll_index(index_win, sections, n_sections, index_scroll, 1);
                    } else {
                        scroll_separator(separator_win, 1);
                    }

                    refresh();
                    wrefresh(content_win);
                }
                break;
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
            case 'j':
                scroll_content(content_win, &content_top, &content_bot, 1);
                break;
            case 'k':
                scroll_content(content_win, &content_top, &content_bot, -1);
                break;
            case 'g':
                top_content(content_win, &content_top, &content_bot);
                break;
            case 'G':
                bot_content(content_win, &content_top, &content_bot);
                break;
            default:
                break;
        }
        ch = getch();
    }
    delwin(content_win);
    endwin();
    free_lines(content_head);
    return 0;
}
