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
#include <time.h>

char *lvl_str[] = {
    "INFO",
    "WARN",
    "ERROR",
    "NIL"
};

void log_(enum log_lvl lvl, char *s) {
    static char *ip = NULL;
    if (!ip) {
        ip = getenv("TCPREMOTEIP");
        if (!ip) {
            ip = "<unknown>";
        }
    }

    if (lvl < 0 || lvl > 3) {
        lvl = 3;
    }

    time_t now;
    time(&now);
    struct tm *tm = gmtime(&now);
    char date[20] = "";
    if (tm) {
        snprintf(date, 20, "%04d-%02d-%02dt%02d-%02d-%02d",
                           tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday,
                           tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
    fprintf(stderr, "%-19s    %-15s    %-5s    %s\n", date, ip, lvl_str[lvl], s);
}

