#ifndef _TELNET_SITE_DATA_H
#define _TELNET_SITE_DATA_H

struct line {
    char *line;
    struct line *prev, *next;
};

struct section {
    char *title;
    char *filename;
    struct line *content;
};

#endif
