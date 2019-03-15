#include "splash.h"

#include "data.h"
#include "render.h"
#include "anim.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

void splash(char *path) {
    signal(SIGWINCH, SIG_IGN);

    struct window splash_window;

    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        splash_window.content.n_raw = read_nlines(fp, &splash_window.content.raw);
        fclose(fp);
    } else {
        fprintf(stderr, "[W] %s:%s:%u: %s: %s\n", binary_name, __FILE__, __LINE__, strerror(errno), path);
        splash_window.content.n_raw = gen_err_opening(&splash_window.content.raw);
    }

    splash_window.window = initscr();
    cbreak();
    halfdelay(1);
    noecho();
    curs_set(0);

    splash_window.cols = COLS;
    splash_window.rows = LINES;
    splash_window.scroll = 0;

    splash_window.content.anim_refs = NULL;
    splash_window.content.n_formatted = flow_nlines(splash_window.content.raw,
                                                    splash_window.content.n_raw,
                                                    &splash_window.content.formatted,
                                                    splash_window.cols);

    render_ncontent(&splash_window);

    refresh();
    wrefresh(splash_window.window);

    int ch = getch();
    while (ch == ERR) {
        anim_tick(&splash_window);
        ch = getch();
    }
    free_content(&splash_window);
    free_anim_refs(&splash_window);
    clear();
    refresh();
    endwin();
    return;
}
