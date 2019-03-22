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

#ifndef TELNET_SITE_MODULE_H
#define TELNET_SITE_MODULE_H

#define QUOTE(s) #s
#define TOSTRING(s) QUOTE(s)

#define INIT_FUNC_NAME module_init
#define SCROLL_FUNC_NAME module_scroll
#define SETMODE_FUNC_NAME module_setmode
#define GETCH_FUNC_NAME module_getch
#define KILL_FUNC_NAME module_kill

#define INIT_FUNC_NAME_S TOSTRING(INIT_FUNC_NAME)
#define SCROLL_FUNC_NAME_S TOSTRING(SCROLL_FUNC_NAME)
#define SETMODE_FUNC_NAME_S TOSTRING(SETMODE_FUNC_NAME)
#define GETCH_FUNC_NAME_S TOSTRING(GETCH_FUNC_NAME)
#define KILL_FUNC_NAME_S TOSTRING(KILL_FUNC_NAME)

#endif
