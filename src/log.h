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

#ifndef _TELNETSITE_LOG_H
#define _TELNETSITE_LOG_H

enum log_lvl {
    LOG_INFO = 0,
    LOG_WARN = 1,
    LOG_ERR = 2,
    LOG_NIL = 3
};

extern char *lvl_str[];

void log_(enum log_lvl lvl, char *fmt, ...);

#endif
