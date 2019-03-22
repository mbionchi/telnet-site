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

#include "data.h"
#include "render.h"
#include "scroll.h"

#include "module.h"

#include <stdlib.h>
#include <ctype.h>
#include <sys/file.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define GUESTBOOK_FILENAME "./guestbook.txt"
#define GUESTBOOK_LOCK_FILENAME "./.guestbook.lock"

#define HANDLE_MAX_LEN 64
#define ENTRY_MAX_LEN 300

#define DISPLAY_MAX_ENTRIES 64

struct window guestbook_window;
enum { DISPLAY_ENTRIES, PROMPT_HANDLE, PROMPT_ENTRY, PROMPT_CONFIRM } guestbook_mode;
char handle_data[HANDLE_MAX_LEN+1];
char entry_data[ENTRY_MAX_LEN+1];
size_t handle_i = 0, entry_i = 0;
time_t entry_time;

size_t gen_display_prompts(struct nline ***nlines) {
    size_t nmemb = 3*DISPLAY_MAX_ENTRIES+10;
    size_t i = 0;
    *nlines = malloc(nmemb*sizeof(struct nline*));
    (*nlines)[i++] = string2nline("guestbook");
    (*nlines)[i++] = string2nline("---------");
    (*nlines)[i++] = string2nline("");
    (*nlines)[i++] = string2nline("  Welcome to the guestbook module!  Here you can leave a short comment as well as read what other visitors wrote.  The controls for this module are as follows:");
    (*nlines)[i++] = string2nline("");
    (*nlines)[i++] = string2nline(" j,k -> scroll the guestbook up or down");
    (*nlines)[i++] = string2nline(" i   -> enter input mode to leave a comment");
    (*nlines)[i++] = string2nline(" ESC -> leave input mode");
    (*nlines)[i++] = string2nline("");
    size_t contains_i = i++;

    size_t n_entries = 0;

    FILE *fp = fopen(GUESTBOOK_FILENAME, "r");
    if (fp != NULL) {
        char *line = NULL;
        size_t line_len = 0;
        ssize_t rv = getline(&line, &line_len, fp);
        while (rv > 0 && n_entries < DISPLAY_MAX_ENTRIES) {
            line[rv-1] = '\0';

            entry_time = atol(line);
            char *timebuf = ctime(&entry_time);
            timebuf[24] = '\0';

            char *orig_line = line;
            line = index(line, ' ')+1;
            size_t handle_len = index(line, ' ') - line;
            char *handle_line = malloc((handle_len+36)*sizeof(char));
            strncpy(handle_line, line, handle_len);
            snprintf(handle_line+handle_len, 36," on %s wrote:", timebuf);

            (*nlines)[i++] = string2nline("");
            (*nlines)[i++] = string2nline(handle_line);
            free(handle_line);

            line = index(line, ' ')+1;
            size_t entry_len = strlen(line);
            char *entry_line = malloc((2+entry_len+1)*sizeof(char));
            snprintf(entry_line, entry_len+3, "  %s", line);

            (*nlines)[i++] = string2nline(entry_line);
            free(entry_line);

            free(orig_line);
            line = NULL;
            line_len = 0;
            rv = getline(&line, &line_len, fp);
            n_entries++;
        }
        if (n_entries > 0) {
            char contains_line[256];
            snprintf(contains_line, 256, "  Displaying the last %d entries:", n_entries);
            (*nlines)[contains_i] = string2nline(contains_line);
        } else {
            (*nlines)[contains_i] = string2nline("  There currently are no entries in the guestbook.");
        }
        fclose(fp);
    } else {
        n_entries = 0;
        int save_errno = errno;
        if (save_errno == ENOENT) {
            (*nlines)[contains_i] = string2nline("  There currently are no entries in the guestbook.");
        } else {
            (*nlines)[contains_i] = string2nline("  There was an error opening the guestbook file.  Sorry about that!");
        }
        fprintf(stderr, "[E] error opening %s: %s\n", GUESTBOOK_FILENAME, strerror(save_errno));
    }
    nmemb = (3*n_entries+10);
    *nlines = realloc(*nlines, nmemb*sizeof(struct nline*));
    return nmemb;
}

size_t gen_handle_prompt(struct nline ***nlines) {
    size_t nmemb = 3;
    *nlines = malloc(nmemb*sizeof(struct nline*));
    (*nlines)[0] = string2nline("Type your handle and press RETURN, or ESCAPE to cancel:");
    (*nlines)[1] = string2nline("");
    (*nlines)[2] = string2nline(" > ");
    return nmemb;
}

size_t gen_entry_prompt(struct nline ***nlines) {
    size_t nmemb = 3;
    *nlines = malloc(nmemb*sizeof(struct nline*));
    (*nlines)[0] = string2nline("Type your guestbook entry (300 characters max) and press RETURN, or ESCAPE to cancel:");
    (*nlines)[1] = string2nline("");
    (*nlines)[2] = string2nline(" > ");
    return nmemb;
}

size_t gen_confirmation_prompt(struct nline ***nlines) {
    size_t nmemb = 1;
    *nlines = malloc(nmemb*sizeof(struct nline*));
    (*nlines)[0] = string2nline("Your entry has been submitted!  Press ESCAPE to return to viewing mode.");
    return nmemb;
}

size_t gen_err_prompt(struct nline ***nlines) {
    size_t nmemb = 1;
    *nlines = malloc(nmemb*sizeof(struct nline*));
    (*nlines)[0] = string2nline("There has been an error submitting your entry.  Press ESCAPE to return to viewing mode.");
    return nmemb;
}

int submit_entry() {
    time(&entry_time);
    int rv = 0;
    int lockfd = open(GUESTBOOK_LOCK_FILENAME, O_RDWR | O_CREAT, 0644);
    if (lockfd != -1) {
        char tmpfilename[256];
        snprintf(tmpfilename, 256, ".guestbook-%d-tmp", getpid());
        int lockrv = flock(lockfd, LOCK_EX);
        if (lockrv == 0) {
            FILE *tmpfp = fopen(tmpfilename, "w");
            if (tmpfp != NULL) {
                FILE *guestbookfp = fopen(GUESTBOOK_FILENAME, "r");
                if (guestbookfp != NULL) {
                    char *line = NULL;
                    size_t line_len = 0;
                    fprintf(tmpfp, "%d %s %s\n", entry_time, handle_data, entry_data);
                    ssize_t rv = getline(&line, &line_len, guestbookfp);
                    while (rv > 0) {
                        fprintf(tmpfp, "%s", line);
                        free(line);
                        line = NULL;
                        line_len = 0;
                        rv = getline(&line, &line_len, guestbookfp);
                    }
                    fclose(guestbookfp);
                }
                rename(tmpfilename, GUESTBOOK_FILENAME);
                fclose(tmpfp);
            } else {
                rv = 1;
            }
            flock(lockfd, LOCK_UN);
        } else {
            rv = 1;
        }
        close(lockfd);
    } else {
        rv = 1;
    }
    return rv;
}

void INIT_FUNC_NAME(WINDOW *window) {
    int x,y;
    getmaxyx(window, guestbook_window.rows, guestbook_window.cols);
    guestbook_window.window = window;
    guestbook_window.content.type = STATIC;
    guestbook_window.content.lines = malloc(sizeof(struct static_content));
    guestbook_window.content.lines->anim_refs = NULL;
    guestbook_window.content.lines->n_raw = gen_display_prompts(&guestbook_window.content.lines->raw);
    guestbook_window.content.lines->n_formatted = flow_nlines(guestbook_window.content.lines->raw,
            guestbook_window.content.lines->n_raw,
            &guestbook_window.content.lines->formatted,
            guestbook_window.cols,
            0);
    guestbook_window.scroll = 0;
    guestbook_mode = DISPLAY_ENTRIES;
    make_scrollable(guestbook_window.window);
    wclear(guestbook_window.window);
    render_ncontent(&guestbook_window);
    wrefresh(guestbook_window.window);
}

void SETMODE_FUNC_NAME(enum mode mode) {
    if (mode == INSERT) {
        memset(handle_data, 0, HANDLE_MAX_LEN+1);
        memset(entry_data, 0, ENTRY_MAX_LEN+1);
        handle_i = 0;
        entry_i = 0;
        guestbook_mode = PROMPT_HANDLE;
        free_content(&guestbook_window);
        guestbook_window.content.lines = malloc(sizeof(struct static_content));
        guestbook_window.content.lines->n_raw = gen_handle_prompt(&guestbook_window.content.lines->raw);
        guestbook_window.content.lines->n_formatted = flow_nlines(guestbook_window.content.lines->raw,
                guestbook_window.content.lines->n_raw,
                &guestbook_window.content.lines->formatted,
                guestbook_window.cols,
                NO_TRAILING_NEWLINE);
        guestbook_window.scroll = 0;
        wclear(guestbook_window.window);
        render_ncontent(&guestbook_window);
        wrefresh(guestbook_window.window);
        curs_set(1);
        wrefresh(guestbook_window.window);
    } else if (mode == COMMAND) {
        curs_set(0);
        guestbook_mode = DISPLAY_ENTRIES;
        free_content(&guestbook_window);

        guestbook_window.content.lines = malloc(sizeof(struct static_content));
        guestbook_window.content.lines->anim_refs = NULL;
        guestbook_window.content.lines->n_raw = gen_display_prompts(&guestbook_window.content.lines->raw);
        guestbook_window.content.lines->n_formatted = flow_nlines(guestbook_window.content.lines->raw,
                guestbook_window.content.lines->n_raw,
                &guestbook_window.content.lines->formatted,
                guestbook_window.cols,
                0);
        guestbook_window.scroll = 0;
        wclear(guestbook_window.window);
        render_ncontent(&guestbook_window);
        wrefresh(guestbook_window.window);
    }
}

void SCROLL_FUNC_NAME(int dy) {
    if (guestbook_mode == DISPLAY_ENTRIES) {
        scroll_ncontent(&guestbook_window, dy);
    }
}

void GETCH_FUNC_NAME(int ch) {
    if (guestbook_mode == PROMPT_HANDLE) {
        if (isgraph(ch)) {
            if (handle_i < HANDLE_MAX_LEN) {
                waddch(guestbook_window.window, ch);
                wrefresh(guestbook_window.window);
                handle_data[handle_i] = (char)ch;
                handle_data[handle_i+1] = '\0';
                handle_i++;
            }
        } else if (ch == '\b' || ch == 127 || ch == 263) {
            if (0 < handle_i) {
                int y,x;
                getyx(guestbook_window.window, y, x);
                if (x == 0) {
                    int maxy, maxx;
                    getmaxyx(guestbook_window.window, maxy, maxx);
                    x = maxx;
                    y--;
                }
                mvwaddch(guestbook_window.window, y, x-1, ' ');
                wmove(guestbook_window.window, y, x-1);
                wrefresh(guestbook_window.window);
                handle_i--;
                handle_data[handle_i] = '\0';
            }
        } else if (ch == '\r') {
            guestbook_mode = PROMPT_ENTRY;
            free_content(&guestbook_window);
            guestbook_window.content.lines = malloc(sizeof(struct static_content));
            guestbook_window.content.lines->n_raw = gen_entry_prompt(&guestbook_window.content.lines->raw);
            guestbook_window.content.lines->n_formatted = flow_nlines(guestbook_window.content.lines->raw,
                    guestbook_window.content.lines->n_raw,
                    &guestbook_window.content.lines->formatted,
                    guestbook_window.cols,
                    NO_TRAILING_NEWLINE);
            guestbook_window.scroll = 0;
            wclear(guestbook_window.window);
            render_ncontent(&guestbook_window);
            wrefresh(guestbook_window.window);
        }
    } else if (guestbook_mode == PROMPT_ENTRY) {
        if (isprint(ch)) {
            if (entry_i < ENTRY_MAX_LEN) {
                waddch(guestbook_window.window, ch);
                wrefresh(guestbook_window.window);
                entry_data[entry_i] = (char)ch;
                entry_i++;
                entry_data[entry_i] = '\0';
            }
        } else if (ch == '\b' || ch == 127 || ch == 263) {
            if (0 < entry_i) {
                int y,x;
                getyx(guestbook_window.window, y, x);
                if (x == 0) {
                    int maxy, maxx;
                    getmaxyx(guestbook_window.window, maxy, maxx);
                    x = maxx;
                    y--;
                }
                mvwaddch(guestbook_window.window, y, x-1, ' ');
                wmove(guestbook_window.window, y, x-1);
                wrefresh(guestbook_window.window);
                entry_i--;
                entry_data[entry_i] = '\0';
            }
        } else if (ch == '\r') {
            guestbook_mode = PROMPT_CONFIRM;
            free_content(&guestbook_window);
            guestbook_window.content.lines = malloc(sizeof(struct static_content));
            int rv = submit_entry();
            if (rv != 0) {
                guestbook_window.content.lines->n_raw = gen_err_prompt(&guestbook_window.content.lines->raw);
            } else {
                guestbook_window.content.lines->n_raw = gen_confirmation_prompt(&guestbook_window.content.lines->raw);
            }
            guestbook_window.content.lines->n_formatted = flow_nlines(guestbook_window.content.lines->raw,
                    guestbook_window.content.lines->n_raw,
                    &guestbook_window.content.lines->formatted,
                    guestbook_window.cols,
                    0);
            guestbook_window.scroll = 0;
            wclear(guestbook_window.window);
            render_ncontent(&guestbook_window);
            wrefresh(guestbook_window.window);
        }
    } else if (guestbook_mode == PROMPT_CONFIRM) {}
}

void KILL_FUNC_NAME(void) {
    free_content(&guestbook_window);
}
