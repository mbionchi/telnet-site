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
 *   along with untrace.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

int index_left_margin = 3;
int separator_left_margin;
int content_right_margin = 8;
int index_n_items;

struct line {
    char *line;
    struct line *prev, *next;
};

struct section {
    char *title;
    char *filename;
    struct line *content;
};

void print_lines(struct line *lines) {
    struct line *iter = lines;
    while (iter) {
        fprintf(stderr, "%s\n", iter->line);
        iter = iter->next;
    }
}

void dump_sections(struct section **sections, size_t n_sections) {
    fprintf(stderr, "%u sections total\n", n_sections);
    for (int i=0; i<n_sections; i++) {
        fprintf(stderr, "%s (%s):\n\n", sections[i]->title, sections[i]->filename);
        print_lines(sections[i]->content);
        fprintf(stderr, "\n");
    }
}

/* TODO: errcheck */
struct line *read_lines(FILE *fp) {
    struct line *head;
    struct line **iter = &head, **prev;
    *iter = malloc(sizeof(struct line));
    char *line = NULL,
         *full_line = NULL;
    size_t line_buf_len = 0,
           full_line_len = 0;
    ssize_t rv = getline(&line, &line_buf_len, fp);
    while (rv != -1) {
        line[rv-1] = '\0';
        if (full_line == NULL) {
            full_line = line;
            full_line_len = rv-1;
        }
        if (full_line[full_line_len-1] == ' ' && full_line != line) {
            full_line = realloc(full_line, (strlen(full_line)+strlen(line)+1)*sizeof(char));
            full_line = strncat(full_line, line, strlen(full_line)+strlen(line));
            full_line_len += rv-1;
            free(line);
        }
        if (full_line[full_line_len-1] != ' ') {
            (*iter) = malloc(sizeof(struct line));
            (*iter)->line = full_line;
            iter = &((*iter)->next);
            full_line = NULL;
        }
        line = NULL;
        line_buf_len = 0;
        rv = getline(&line, &line_buf_len, fp);
    }
    *iter = NULL;
    return head;
}

int compare_sections(const void *a, const void *b) {
    struct section *s1 = *(struct section**)a;
    struct section *s2 = *(struct section**)b;
    return strcmp(s1->filename, s2->filename);
}

/* TODO: errcheck */
struct section **read_sections(DIR *dir, char *dirname, int follow_links, size_t *nmemb) {
    int linear_step = 8;
    *nmemb = 8;
    struct section **base = malloc(*nmemb*sizeof(struct section*));
    struct dirent *dirent = readdir(dir);
    int i=0;
    while (dirent != NULL) {
        if (dirent->d_name[0] != '.') {
            struct stat sb;
            char *filename = malloc((strlen(dirname)+strlen(dirent->d_name)+2)*sizeof(char));
            sprintf(filename, "%s/%s", dirname, dirent->d_name);
            if (follow_links) {
                stat(filename, &sb);
            } else {
                lstat(filename, &sb);
            }
            if ((sb.st_mode & S_IFMT) == S_IFREG || (sb.st_mode & S_IFMT) == S_IFDIR) {
                if (i >= *nmemb) {
                    *nmemb += linear_step;
                    base = realloc(base, *nmemb*sizeof(struct section*));
                }
                base[i] = malloc(sizeof(struct section));
                char *title = index(dirent->d_name, '-')+1;
                if (!title) {
                    title = dirent->d_name;
                }
                size_t title_len = strlen(title);
                base[i]->title = strcpy(malloc((title_len+1)*sizeof(char)), title);
                base[i]->filename = filename;
                i++;
            }
        }
        dirent = readdir(dir);
    }
    *nmemb = i;
    qsort(base, *nmemb, sizeof(struct section*), &compare_sections);
    return base;
}

void render_separator(int n, int selected) {
    selected++;
    int i;
    for (i=0; i<n; i++) {
        if (i == selected) {
            mvprintw(i, separator_left_margin, "< ");
        } else {
            mvprintw(i, separator_left_margin, " |");
        }
    }
}

void render_index(struct section **sections, size_t n_sections) {
    size_t max_len = 0;
    int i;
    for (i=0; i<n_sections; i++) {
        size_t title_len = strlen(sections[i]->title);
        if (title_len > max_len) {
            max_len = title_len;
        }
    }
    for (i=0; i<n_sections; i++) {
        size_t title_len = strlen(sections[i]->title);
        mvprintw(i+1, index_left_margin+max_len-title_len, "%s", sections[i]->title);
    }
    separator_left_margin = index_left_margin+max_len+1;
    index_n_items = i-1;
}

void free_lines(struct line *lines) {
    struct line *iter = lines;
    struct line *prev;
    while (iter) {
        prev = iter;
        iter = iter->next;
        free(prev->line);
        free(prev);
    }
}

/*
 * this flow algorithm has:
 *   - word wrapping
 *   - list indentation, for example
 *     like this <-
 *   - a bug that will SIGSEGV this
 *     program if a contigous word is
 *     longer than `width`
 */
struct line *flow_content(struct line *lines, int width) {
    struct line *rv_head = malloc(sizeof(struct line));
    struct line *rv_iter = rv_head;
    struct line *rv_prev = NULL;
    rv_iter->prev = NULL;
    struct line *iter = lines;
    while (iter) {
        char *head = iter->line;
        size_t head_len = strlen(head);
        unsigned prepend_space;
        int found_non_whitespace = 0;
        for (prepend_space=0;
             prepend_space<head_len && !isalpha(head[prepend_space]);
             prepend_space++) {
            if (!found_non_whitespace && !isspace(head[prepend_space])) {
                found_non_whitespace = 1;
            }
        }
        if (!found_non_whitespace || prepend_space == head_len) {
            prepend_space = 0;
        }
        char *tail = head;
        int split_index, prev_split_index;
        while (tail != NULL) {
            tail = index(head, ' ');
            split_index = tail-head;
            prev_split_index = split_index;
            while (tail != NULL && split_index+prepend_space < width) {
                prev_split_index = split_index;
                tail = index(tail+1, ' ');
                split_index = tail-head;
            }
            if (tail != NULL || strlen(head)+prepend_space > width) {
                head[prev_split_index] = '\0';
                if (head == iter->line) {
                    rv_iter->line = strcpy(malloc((strlen(head)+1)*sizeof(char)), head);
                } else {
                    rv_iter->line = malloc((strlen(head)+prepend_space+1)*sizeof(char));
                    for (int i=0; i<prepend_space; i++) {
                        rv_iter->line[i] = ' ';
                    }
                    strcpy(rv_iter->line+prepend_space, head);
                }
                rv_iter->next = malloc(sizeof(struct line));
                rv_prev = rv_iter;
                rv_iter = rv_iter->next;
                rv_iter->prev = rv_prev;
                head[prev_split_index] = ' ';
                head += prev_split_index+1;
            }
        }
        if (head == iter->line) {
            rv_iter->line = strcpy(malloc((strlen(head)+1)*sizeof(char)), head);
        } else {
            rv_iter->line = malloc((strlen(head)+prepend_space+1)*sizeof(char));
            for (int i=0; i<prepend_space; i++) {
                rv_iter->line[i] = ' ';
            }
            strcpy(rv_iter->line+prepend_space, head);
        }
        rv_iter->next = malloc(sizeof(struct line));
        rv_prev = rv_iter;
        rv_iter = rv_iter->next;
        rv_iter->prev = rv_prev;
        iter = iter->next;
    }
    free(rv_prev->next);
    rv_prev->next = NULL;
    return rv_head;
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

/*
 * TODO:
 *   - WINCH
 *   - scroll file list
 *   - figure out why scrolling fails over telnet ?
 *   - error handling everywhere
 */
int main(int argc, char **argv) {
    int selected_index = 0;
    int scroll = 0;
    size_t n_sections;
    DIR *dir = opendir(argv[1]);
    struct section **sections = read_sections(dir, argv[1], 1, &n_sections);
    closedir(dir);
    FILE *fp = fopen(sections[selected_index]->filename, "r");
    sections[selected_index]->content = read_lines(fp);
    fclose(fp);
    initscr();
    raw();
    noecho();
    curs_set(0);
    render_index(sections, n_sections);
    render_separator(n_sections+2, selected_index);
    WINDOW *content_win;
    int content_rows = LINES;
    int content_cols = COLS-separator_left_margin-4-content_right_margin;
    content_win = newwin(content_rows, content_cols, 0, separator_left_margin+4);
    scrollok(content_win, 1);
    wsetscrreg(content_win, 0, LINES-1);
    struct line *content = flow_content(sections[selected_index]->content, content_cols);
    struct line *content_top = content;
    struct line *content_bot = content;
    for (int i=0; i<content_rows-1 && content_bot->next != NULL; i++) {
        content_bot = content_bot->next;
    }
    render_content(content_win, content);
    refresh();
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
                    render_separator(n_sections+2, selected_index);
                    wclear(content_win);
                    free_lines(content);
                    content = flow_content(sections[selected_index]->content, content_cols);
                    content_top = content;
                    content_bot = content;
                    for (int i=0; i<content_rows-1 && content_bot->next != NULL; i++) {
                        content_bot = content_bot->next;
                    }
                    render_content(content_win, content);
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
                    render_separator(n_sections+2, selected_index);
                    wclear(content_win);
                    free_lines(content);
                    content = flow_content(sections[selected_index]->content, content_cols);
                    content_top = content;
                    content_bot = content;
                    for (int i=0; i<content_rows-1 && content_bot->next != NULL; i++) {
                        content_bot = content_bot->next;
                    }
                    render_content(content_win, content);
                    refresh();
                    wrefresh(content_win);
                }
                break;
            case 'j':
                if (content_bot->next != NULL) {
                    wscrl(content_win, 1);
                    content_top = content_top->next;
                    content_bot = content_bot->next;
                    mvwprintw(content_win, content_rows-1, 0, "%s", content_bot->line);
                    wrefresh(content_win);
                }
                break;
            case 'k':
                if (content_top->prev != NULL) {
                    wscrl(content_win, -1);
                    content_top = content_top->prev;
                    content_bot = content_bot->prev;
                    mvwprintw(content_win, 0, 0, "%s", content_top->line);
                    wrefresh(content_win);
                }
                break;
            case 'g':
                if (content_top->prev != NULL) {
                    content_top = content;
                    content_bot = content;
                    for (int i=0; i<content_rows-1 && content_bot->next != NULL; i++) {
                        content_bot = content_bot->next;
                    }
                    wclear(content_win);
                    render_content(content_win, content);
                    wrefresh(content_win);
                }
                break;
            case 'G':
                if (content_bot->next != NULL) {
                    while (content_bot->next != NULL) {
                        content_top = content_top->next;
                        content_bot = content_bot->next;
                    }
                    wclear(content_win);
                    render_content(content_win, content_top);
                    wrefresh(content_win);
                }
                break;
            default:
                break;
        }
        ch = getch();
    }
    delwin(content_win);
    endwin();
    free_lines(content);
    return 0;
}
