#ifndef _TELNET_SITE_RENDER_H
#define _TELNET_SITE_RENDER_H

#include "data.h"
#include <ncurses.h>

#define SEPARATOR_SELECTED " <   "
#define SEPARATOR_REGULAR  "  |  "

void render_index(WINDOW *window, struct section **sections, size_t n_sections);
void render_separator(WINDOW *window, int selected_index);
void render_content(WINDOW *window, struct line *lines);

#endif
