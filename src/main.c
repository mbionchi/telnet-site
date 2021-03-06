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
#include "splash.h"
#include "data.h"
#include "winch.h"
#include "log.h"

#include <getopt.h>
#include <stdlib.h>
#include <locale.h>
#include <ncurses.h>

enum { SITE_INDEX, SPLASH_INDEX };

int main(int argc, char **argv) {
    binary_name = argv[0];
    char *splash_path = NULL,
         *site_path = NULL;
    static struct option options[] = {
        {"site",   required_argument, NULL, SITE_INDEX},
        {"splash", required_argument, NULL, SPLASH_INDEX},
        {NULL, 0, NULL, 0}
    };
    int opt = getopt_long_only(argc, argv, "", options, NULL);
    while (opt != -1) {
        switch (opt) {
            case SITE_INDEX:
                site_path = optarg;
                break;
            case SPLASH_INDEX:
                splash_path = optarg;
                break;
        }
        opt = getopt_long_only(argc, argv, "", options, NULL);
    }

    log_(LOG_INFO, "started session, TERM is %s", getenv("TERM"));
    if (!site_path) {
        log_(LOG_ERR, "usage: %s --site <path-to-dir> [--splash <path-to-file>]", binary_name);
        exit(1);
    }

    had_winch = 0;
    signal(SIGWINCH, set_had_winch);

    setlocale(LC_ALL, "");
    WINDOW *main_window = initscr();
    cbreak();
    halfdelay(1);
    noecho();
    int curs_save = curs_set(0);
    nonl();
    keypad(main_window, 1);

    int rv = 0;
    if (splash_path) {
        rv |= splash(splash_path);
    }
    if (site_path) {
        rv |= site(site_path);
    }

    curs_set(curs_save);
    endwin();
    log_(rv?LOG_ERR:LOG_INFO, "ended session with status %d", rv);
    return rv;
}
