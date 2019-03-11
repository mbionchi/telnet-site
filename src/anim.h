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

#ifndef _TELNET_SITE_ANIM_H
#define _TELNET_SITE_ANIM_H

#include "data.h"

void anim_tick(struct window *window);
void push_anim_ref_front(struct window *window, size_t index);
void push_anim_ref_back(struct window *window, size_t index);
void pop_anim_ref_front(struct window *window);
void pop_anim_ref_back(struct window *window);
void free_anim_refs(struct window *window);

#endif
