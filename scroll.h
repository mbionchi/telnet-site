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
