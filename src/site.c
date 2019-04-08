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
#include "winch.h"

#include <errno.h>
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

int reload_content = 0;

/*
 * NOTE:
 *   scrollok(window, 0) needs to be called every time before
 *   doing mvwprintw to avoid forced scroll caused by the
 *   cursor possibly reacing the end of screen, and scrollok(window, 1)
 *   needs to be called after the mvwprintw call.
 * TODO:
 *   - error handling everywhere please
 *   - refactor the event loop, it's horrifying atm
 */
void site(char *path) {
    struct window index_window;
    struct window separator_window;
    struct window content_window;

    enum mode mode = COMMAND;

    int selected_index = 0;
    int scroll = 0;
    size_t n_sections;
    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "[E] %s:%s:%u: %s: %s\n", binary_name, __FILE__, __LINE__, strerror(errno), path);
        exit(1);
    }
    struct section **sections = read_sections(dir, path, &n_sections);
    closedir(dir);

    int rv = read_content_from_section(&content_window.content, sections[selected_index]);
    if (rv != 0) {
        gen_err_opening(&content_window.content);
    }

    int index_left_margin = 2;
    int index_cols = get_index_width(sections, n_sections);
    int index_scroll_threshold = 5;

    int separator_cols = strlen(SEPARATOR_SELECTED);
    int separator_rows = LINES;
    int separator_left_margin = index_left_margin + index_cols;

    int content_left_margin = index_left_margin + index_cols + separator_cols;
    int content_right_margin = 8;

    index_window.cols = get_index_width(sections, n_sections);
    index_window.rows = LINES;
    index_window.scroll = 0;
    index_window.window = newwin(index_window.rows, index_window.cols, 0, index_left_margin);

    index_window.content.type = STATIC;
    index_window.content.lines = malloc(sizeof(struct static_content));
    index_window.content.lines->anim_refs = NULL;
    index_window.content.lines->n_formatted = gen_index(sections,
                                                        n_sections,
                                                        &index_window.content.lines->formatted,
                                                        index_window.cols);

    separator_window.cols = strlen(SEPARATOR_SELECTED);
    separator_window.rows = LINES;
    separator_window.scroll = 1;
    separator_window.window = newwin(separator_window.rows, separator_window.cols, 0, separator_left_margin);

    content_window.cols = COLS - content_left_margin - content_right_margin;
    content_window.rows = LINES;
    content_window.scroll = 0;
    content_window.window = newwin(content_window.rows, content_window.cols, 0, content_left_margin);

    if (content_window.content.type == STATIC && content_window.content.lines != NULL) {
        content_window.content.lines->anim_refs = NULL;
        content_window.content.lines->n_formatted = flow_nlines(content_window.content.lines->raw,
                                                                content_window.content.lines->n_raw,
                                                                &content_window.content.lines->formatted,
                                                                content_window.cols,
                                                                0);
    }

    make_scrollable(index_window.window);
    make_scrollable(separator_window.window);
    if (content_window.content.type == STATIC && content_window.content.lines != NULL) {
        make_scrollable(content_window.window);
    }

    render_ncontent(&index_window);
    render_separator(&separator_window);
    if (content_window.content.type == STATIC && content_window.content.lines != NULL) {
        render_ncontent(&content_window);
    }

    refresh();
    wrefresh(index_window.window);
    wrefresh(separator_window.window);
    if (content_window.content.type == STATIC && content_window.content.lines != NULL) {
        wrefresh(content_window.window);
    } else if (content_window.content.type == DYNAMIC && content_window.content.dlib != NULL) {
        content_window.content.dlib->init_fun(content_window.window);
    }

    int ch = getch();
    while (ch != 'q' || mode == INSERT) {
        switch (ch) {
            case 'i':
                if (mode == INSERT &&
                        content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                } else if (content_window.content.type == DYNAMIC &&
                           content_window.content.dlib != NULL &&
                           content_window.content.dlib->setmode_fun != NULL) {
                    mode = INSERT;
                    content_window.content.dlib->setmode_fun(INSERT);
                }
                break;
            /*
             * '\e' is non-standard, but should be supported by most compilers.  it stands
             * for ESC
             */
            case '\e':
                if (mode != COMMAND) {
                    mode = COMMAND;
                    if (content_window.content.type == DYNAMIC &&
                            content_window.content.dlib != NULL &&
                            content_window.content.dlib->setmode_fun != NULL) {
                        content_window.content.dlib->setmode_fun(COMMAND);
                    }
                }
                break;
            case 'J':
                if (mode == COMMAND) {
                    if (selected_index < n_sections-1) {
                        selected_index++;
                        reload_content = 1;
                        if (separator_window.scroll + index_scroll_threshold < separator_window.rows ||
                                index_window.content.lines->n_formatted <= index_window.scroll + index_window.rows) {
                            scroll_separator(&separator_window, 1);
                        } else {
                            scroll_ncontent(&index_window, 1);
                        }
                    }
                } else if (mode == INSERT &&
                        content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                }
                break;
            case 'K':
                if (mode == COMMAND) {
                    if (selected_index > 0) {
                        selected_index--;
                        reload_content = 1;
                        if (separator_window.scroll >= index_scroll_threshold ||
                                index_window.scroll == 0) {
                            scroll_separator(&separator_window, -1);
                        } else {
                            scroll_ncontent(&index_window, -1);
                        }
                    }
                } else if (mode == INSERT &&
                        content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                }
                break;
            case KEY_DOWN:
            case 'j':
                if (mode == COMMAND) {
                    if (content_window.content.type == STATIC) {
                        scroll_ncontent(&content_window, 1);
                    } else if (content_window.content.type == DYNAMIC &&
                               content_window.content.dlib != NULL &&
                               content_window.content.dlib->scroll_fun != NULL) {
                        content_window.content.dlib->scroll_fun(1);
                    }
                } else if (mode == INSERT &&
                           content_window.content.type == DYNAMIC &&
                           content_window.content.dlib != NULL &&
                           content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                }
                break;
            case KEY_UP:
            case 'k':
                if (mode == COMMAND) {
                    if (content_window.content.type == STATIC) {
                        scroll_ncontent(&content_window, -1);
                    } else if (content_window.content.type == DYNAMIC &&
                               content_window.content.dlib != NULL &&
                               content_window.content.dlib->scroll_fun != NULL) {
                        content_window.content.dlib->scroll_fun(-1);
                    }
                } else if (mode == INSERT &&
                        content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                }
                break;
            case KEY_HOME:
            case 'g':
                if (mode == COMMAND && content_window.content.type == STATIC) {
                    if (content_window.scroll > 0) {
                        wclear(content_window.window);
                        content_window.scroll = 0;
                        render_ncontent(&content_window);
                        wrefresh(content_window.window);
                    }
                } else if (mode == INSERT &&
                        content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                }
                break;
            case KEY_END:
            case 'G':
                if (mode == COMMAND && content_window.content.type == STATIC) {
                    if (content_window.content.lines->n_formatted > content_window.rows) {
                        wclear(content_window.window);
                        content_window.scroll = content_window.content.lines->n_formatted - content_window.rows;
                        render_ncontent(&content_window);
                        wrefresh(content_window.window);
                    }
                } else if (mode == INSERT &&
                        content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                }
                break;
            case ERR:
                if (content_window.content.type == STATIC && content_window.content.lines != NULL) {
                    anim_tick(&content_window);
                    if (had_winch) {
                        delwin(index_window.window);
                        delwin(separator_window.window);
                        delwin(content_window.window);

                        endwin();
                        refresh();

                        index_window.rows = LINES;
                        index_window.window = newwin(index_window.rows, index_window.cols, 0, index_left_margin);

                        separator_window.rows = LINES;
                        separator_window.scroll = selected_index+1;
                        separator_window.window = newwin(separator_window.rows, separator_window.cols, 0, separator_left_margin);

                        content_window.cols = COLS - content_left_margin - content_right_margin;
                        content_window.rows = LINES;
                        content_window.scroll = 0;
                        content_window.window = newwin(content_window.rows, content_window.cols, 0, content_left_margin);

                        free_nlines(content_window.content.lines->formatted, content_window.content.lines->n_formatted);
                        free_anim_refs(&content_window);

                        content_window.content.lines->n_formatted =
                            flow_nlines(content_window.content.lines->raw,
                                        content_window.content.lines->n_raw,
                                        &content_window.content.lines->formatted,
                                        content_window.cols,
                                        0);

                        make_scrollable(index_window.window);
                        make_scrollable(separator_window.window);
                        make_scrollable(content_window.window);

                        render_ncontent(&index_window);
                        render_separator(&separator_window);
                        render_ncontent(&content_window);


                        refresh();

                        wrefresh(index_window.window);
                        wrefresh(separator_window.window);
                        wrefresh(content_window.window);

                        had_winch = 0;
                    }
                }
                if (reload_content) {
                    if (content_window.content.type == STATIC) {
                        free_content(&content_window);
                        free_anim_refs(&content_window);
                    } else if (content_window.content.type == DYNAMIC &&
                               content_window.content.dlib != NULL &&
                               content_window.content.dlib->kill_fun != NULL) {
                        content_window.content.dlib->kill_fun();
                    }

                    rv = read_content_from_section(&content_window.content, sections[selected_index]);
                    if (rv != 0) {
                        gen_err_opening(&content_window.content);
                    }

                    if (content_window.content.type == STATIC && content_window.content.lines != NULL) {
                        content_window.content.lines->n_formatted =
                            flow_nlines(content_window.content.lines->raw,
                                        content_window.content.lines->n_raw,
                                        &content_window.content.lines->formatted,
                                        content_window.cols,
                                        0);
                    }
                    content_window.scroll = 0;

                    if (content_window.content.type == STATIC && content_window.content.lines != NULL) {
                        wclear(content_window.window);
                        render_ncontent(&content_window);
                        wrefresh(content_window.window);
                    } else if (content_window.content.type == DYNAMIC && content_window.content.dlib != NULL) {
                        content_window.content.dlib->init_fun(content_window.window);
                    }

                    reload_content = 0;
                }
                if (content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ERR);
                }
                break;
            default:
                if (content_window.content.type == DYNAMIC &&
                        content_window.content.dlib != NULL &&
                        content_window.content.dlib->getch_fun != NULL) {
                    content_window.content.dlib->getch_fun(ch);
                }
                break;
        }
        ch = getch();
    }
    free_content(&content_window);
    free_anim_refs(&content_window);
    delwin(index_window.window);
    delwin(separator_window.window);
    delwin(content_window.window);
}
