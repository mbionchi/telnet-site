#ifndef _TELNET_SITE_SCROLL_H
#define _TELNET_SITE_SCROLL_H

#include "data.h"
#include <ncurses.h>

void make_scrollable(WINDOW *window);
void scroll_content(WINDOW *window, struct line **content_top, struct line **content_bot, int dy);
void top_content(WINDOW *window, struct line **content_top, struct line **content_bot);
void bot_content(WINDOW *window, struct line **content_top, struct line **content_bot);
void scroll_index(WINDOW *window, struct section **sections, size_t n_sections, int index_scroll, int dy);
void scroll_separator(WINDOW *window, int dy);

#endif
