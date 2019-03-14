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

#ifndef _TELNET_SITE_RENDER_H
#define _TELNET_SITE_RENDER_H

#include "data.h"
#include <ncurses.h>

#define SEPARATOR_SELECTED " <   "
#define SEPARATOR_REGULAR  "  |  "

void render_separator(struct window *window);
void render_ncontent(struct window *window);
void render_nline(WINDOW *window, size_t cursor_y, struct nline *nline);

#endif
