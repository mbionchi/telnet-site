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

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define LOGPATH "/tmp/telnetsite"

void errlog(char *fmt, ...) {
    static FILE *fp = NULL;
    if (fp == NULL) {
        fp = fopen(LOGPATH, "a");
    }
    if (fp != NULL) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        fflush(fp);
        va_end(args);
    }
}

