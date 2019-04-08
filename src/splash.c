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

#include "splash.h"

#include "data.h"
#include "render.h"
#include "anim.h"
#include "winch.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ncurses.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

void splash(char *path) {
    struct window splash_window;

    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        splash_window.content.type = STATIC;
        splash_window.content.lines = malloc(sizeof(struct static_content));
        splash_window.content.lines->n_raw = read_nlines(fp, &splash_window.content.lines->raw);
        fclose(fp);
    } else {
        fprintf(stderr, "[W] %s:%s:%u: %s: %s\n", binary_name, __FILE__, __LINE__, strerror(errno), path);
        gen_err_opening(&splash_window.content);
    }

    splash_window.window = newwin(0,0,0,0);

    splash_window.cols = COLS;
    splash_window.rows = LINES;
    splash_window.scroll = 0;

    splash_window.content.lines->anim_refs = NULL;
    splash_window.content.lines->n_formatted = flow_nlines(splash_window.content.lines->raw,
                                                    splash_window.content.lines->n_raw,
                                                    &splash_window.content.lines->formatted,
                                                    splash_window.cols,
                                                    0);

    render_ncontent(&splash_window);

    wrefresh(splash_window.window);

    int ch = getch();
    while (ch == ERR) {
        if (splash_window.content.type == STATIC) {
            anim_tick(&splash_window);
            if (had_winch) {
                delwin(splash_window.window);
                endwin();
                refresh();
                splash_window.window = newwin(0,0,0,0);
                splash_window.cols = COLS;
                splash_window.rows = LINES;
                free_nlines(splash_window.content.lines->formatted, splash_window.content.lines->n_formatted);
                free_anim_refs(&splash_window);
                splash_window.content.lines->n_formatted = flow_nlines(splash_window.content.lines->raw, splash_window.content.lines->n_raw, &splash_window.content.lines->formatted, splash_window.cols, 0);
                render_ncontent(&splash_window);
                refresh();
                wrefresh(splash_window.window);
                had_winch = 0;
            }
            wrefresh(splash_window.window);
        }
        ch = getch();
    }
    free_content(&splash_window);
    free_anim_refs(&splash_window);
    clear();
    wrefresh(splash_window.window);
    delwin(splash_window.window);
    return;
}
