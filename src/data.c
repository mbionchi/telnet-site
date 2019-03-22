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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.h"

#include "module.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define ERR_OPENING_FILE_STR "<error opening file>"

void dump_sections(struct section **sections, size_t n_sections) {
    fprintf(stderr, "%u sections total\n", n_sections);
    for (int i=0; i<n_sections; i++) {
        fprintf(stderr, "%s (%s):\n\n", sections[i]->title, sections[i]->filename);
    }
}

int compare_sections(const void *a, const void *b) {
    struct section *s1 = *(struct section**)a;
    struct section *s2 = *(struct section**)b;
    return strcmp(s1->filename, s2->filename);
}

int read_content_from_section(struct content *content, struct section *section) {
    if (section->type == STATIC) {
        content->type = STATIC;
        content->lines = malloc(sizeof(struct static_content));
        content->lines->anim_refs = NULL;
        FILE *fp = fopen(section->filename, "r");
        if (fp != NULL) {
            content->lines->n_raw = read_nlines(fp, &content->lines->raw);
            fclose(fp);
        } else {
            fprintf(stderr, "[W] %s:%s:%u: %s: %s\n", binary_name, __FILE__, __LINE__, strerror(errno), section->filename);
            free(content->lines);
            content->lines = NULL;
            return 1;
        }
    } else if (section->type = DYNAMIC) {
        content->type = DYNAMIC;
        content->dlib = malloc(sizeof(struct dynamic_content));
        content->dlib->so_handle = dlopen(section->filename, RTLD_LAZY);
        if (content->dlib->so_handle == NULL) {
            fprintf(stderr, "[W] %s:%s:%u: %s: %s\n", binary_name, __FILE__, __LINE__, dlerror());
            free(content->dlib);
            content->dlib = NULL;
            return 1;
        } else {
            content->dlib->init_fun = dlsym(content->dlib->so_handle, INIT_FUNC_NAME_S);
            if (content->dlib->init_fun == NULL) {
                fprintf(stderr, "[W] %s:%s:%u: %s: %s\n", binary_name, __FILE__, __LINE__, dlerror());
                dlclose(content->dlib->so_handle);
                free(content->dlib);
                content->dlib = NULL;
                return 1;
            } else {
                content->dlib->getch_fun = dlsym(content->dlib->so_handle, GETCH_FUNC_NAME_S);
                content->dlib->scroll_fun = dlsym(content->dlib->so_handle, SCROLL_FUNC_NAME_S);
                content->dlib->setmode_fun = dlsym(content->dlib->so_handle, SETMODE_FUNC_NAME_S);
                content->dlib->kill_fun = dlsym(content->dlib->so_handle, KILL_FUNC_NAME_S);
            }
        }
    }
    return 0;
}

/* TODO: errcheck */
struct section **read_sections(DIR *dir, char *dirname, size_t *nmemb) {
    int linear_step = 8;
    *nmemb = 8;
    struct section **base = malloc(*nmemb*sizeof(struct section*));
    struct dirent *dirent = readdir(dir);
    int i=0;
    while (dirent != NULL) {
        if (dirent->d_name[0] != '.') {
            struct stat sb;
            char *filename = malloc((strlen(dirname)+strlen(dirent->d_name)+2)*sizeof(char));
            sprintf(filename, "%s/%s", dirname, dirent->d_name);
#ifdef FOLLOW_LINKS
            stat(filename, &sb);
#else
            lstat(filename, &sb);
#endif
            if ((sb.st_mode & S_IFMT) == S_IFREG || (sb.st_mode & S_IFMT) == S_IFDIR) {
                if (i >= *nmemb) {
                    *nmemb += linear_step;
                    base = realloc(base, *nmemb*sizeof(struct section*));
                }
                base[i] = malloc(sizeof(struct section));
                char *title = index(dirent->d_name, '-');
                if (!title) {
                    title = dirent->d_name;
                } else {
                    title++;
                }
                size_t title_len = strlen(title);
                if (strcmp(".so", title+title_len-3) == 0) {
                    base[i]->type = DYNAMIC;
                    title_len -= 3;
                } else {
                    base[i]->type = STATIC;
                }
                base[i]->title = strncpy(malloc((title_len+1)*sizeof(char)), title, title_len);
                base[i]->title[title_len] = '\0';
                base[i]->filename = filename;
                i++;
            }
        }
        dirent = readdir(dir);
    }
    *nmemb = i;
    qsort(base, *nmemb, sizeof(struct section*), &compare_sections);
    return base;
}

// big todo: errcheck mallocs, strncpys, getlines

struct nline *mkblank() {
    struct nline *blank = malloc(sizeof(struct nline));
    blank->type = TEXT;
    blank->text = malloc(sizeof(struct string));
    blank->text->len = 0;
    blank->text->data = strcpy(malloc(1*sizeof(char)), "");
    return blank;
}

void gen_err_opening(struct content *content) {
    content->type = STATIC;
    content->lines = malloc(sizeof(struct static_content));
    content->lines->n_raw = 1;
    content->lines->raw = malloc(sizeof(struct nline*));
    content->lines->raw[0] = malloc(sizeof(struct nline));
    content->lines->raw[0]->type = TEXT;
    content->lines->raw[0]->text = malloc(sizeof(struct string));
    content->lines->raw[0]->text->data = strcpy(malloc((strlen(ERR_OPENING_FILE_STR)+1)*sizeof(char)), ERR_OPENING_FILE_STR);
    content->lines->raw[0]->text->len = strlen(ERR_OPENING_FILE_STR);
}

struct nline *string2nline(char *str) {
    struct nline *nline = malloc(sizeof(struct nline));
    nline->type = TEXT;
    nline->text = malloc(sizeof(struct string));
    nline->text->len = strlen(str);
    nline->text->data = strcpy(malloc((nline->text->len+1)*sizeof(char)), str);
    return nline;
}

size_t read_nlines(FILE *fp, struct nline ***nlines) {
    size_t n_raw = 0;
    size_t nmemb = 8,
           linear_step = 8;
    *nlines = malloc(nmemb*sizeof(struct nline*));
    char *line = NULL,
         *full_line = NULL;
    size_t line_buf_len = 0,
           full_line_len = 0;
    ssize_t rv = getline(&line, &line_buf_len, fp);
    while (rv != -1) {
        line[rv-1] = '\0';
        if (full_line == NULL) {
            if (!strcmp(line, ANIM_HINT)) {
                size_t n_anim_lines = 0;
                size_t anim_line_index = n_raw;
                size_t n_frames = 0;
                free(line);
                line = NULL;
                line_buf_len = 0;
                rv = getline(&line, &line_buf_len, fp);
                line[rv-1] = '\0';
                while (strcmp(line, LOOP_HINT) && strcmp(line, NOLOOP_HINT)) {
                    if (!strncmp(line, FRAME_HINT, strlen(FRAME_HINT))) {
                        size_t frame_delay = atoi(line+strlen(FRAME_HINT));
                        free(line);
                        line = NULL;
                        line_buf_len = 0;
                        rv = getline(&line, &line_buf_len, fp);
                        line[rv-1] = '\0';
                        while (strncmp(line, FRAME_HINT, strlen(FRAME_HINT)) &&
                                strcmp(line, LOOP_HINT) && strcmp(line, NOLOOP_HINT)) {
                            if (n_frames < 1) {
                                struct nline *nline = malloc(sizeof(struct nline));
                                nline->type = ANIM;
                                nline->anim = malloc(sizeof(struct animation));
                                nline->anim->nmemb_frames = 8;
                                nline->anim->frames = malloc(nline->anim->nmemb_frames*sizeof(struct frame));
                                nline->anim->frames[0].s = malloc(sizeof(struct string));
                                nline->anim->frames[0].s->data = line;
                                nline->anim->frames[0].s->len = rv-1;
                                nline->anim->frames[0].delay = frame_delay;
                                nline->anim->current_frame_index = 0;
                                nline->anim->n_frames = 1;
                                if (n_anim_lines == 0) {
                                    nline->anim->is_first_line = 1;
                                } else {
                                    nline->anim->is_first_line = 0;
                                }
                                if (anim_line_index >= nmemb) {
                                    nmemb += linear_step;
                                    *nlines = realloc(*nlines, nmemb*sizeof(struct nline*));
                                }
                                (*nlines)[anim_line_index] = nline;
                                anim_line_index++;
                                n_anim_lines++;
                            } else if (anim_line_index < n_anim_lines+n_raw) {
                                struct animation *anim = (*nlines)[anim_line_index]->anim;
                                if (anim->n_frames >= anim->nmemb_frames) {
                                    anim->nmemb_frames += linear_step;
                                    anim->frames = realloc(anim->frames, anim->nmemb_frames*(sizeof(struct frame)));
                                }
                                struct string *s = malloc(sizeof(struct string));
                                s->data = line;
                                s->len = rv-1;
                                anim->frames[anim->n_frames].s = s;
                                anim->frames[anim->n_frames].delay = frame_delay;
                                anim->n_frames++;
                                anim_line_index++;
                            }
                            line = NULL;
                            line_buf_len = 0;
                            rv = getline(&line, &line_buf_len, fp);
                            line[rv-1] = '\0';
                        }
                        n_frames++;
                        anim_line_index -= n_anim_lines;
                    } else {
                        line = NULL;
                        line_buf_len = 0;
                        rv = getline(&line, &line_buf_len, fp);
                        line[rv-1] = '\0';
                    }
                }
                if (!strcmp(line, LOOP_HINT)) {
                    (*nlines)[n_raw]->anim->loop = 1;
                } else if (!strcmp(line, NOLOOP_HINT)) {
                    (*nlines)[n_raw]->anim->loop = 0;
                }
                n_raw += n_anim_lines;
                goto next_line;
            } else {
                full_line = line;
                full_line_len = rv-1;
            }
        }
        if (full_line != NULL && full_line_len > 0 && full_line[full_line_len-1] == ' ' && full_line != line) {
            size_t new_full_line_len = full_line_len + rv-1;
            full_line = realloc(full_line, (new_full_line_len+1)*sizeof(char));
            full_line = strncat(full_line, line, new_full_line_len);
            full_line_len = new_full_line_len;
            free(line);
        }
        if (full_line != NULL && (full_line_len == 0 || full_line[full_line_len-1] != ' ')) {
            struct nline *nline = malloc(sizeof(struct nline));
            nline->type = TEXT;
            nline->text = malloc(sizeof(struct string));
            nline->text->data = full_line;
            nline->text->len = full_line_len;
            if (n_raw >= nmemb) {
                nmemb += linear_step;
                *nlines = realloc(*nlines, nmemb*sizeof(struct nline*));
            }
            (*nlines)[n_raw] = nline;
            n_raw++;
            full_line = NULL;
        }
next_line:
        line = NULL;
        line_buf_len = 0;
        rv = getline(&line, &line_buf_len, fp);
    }
    return n_raw;
}

size_t flow_nlines(struct nline **from, size_t n_from, struct nline ***to, int width, int options) {
    size_t n_to = 0;
    size_t nmemb = 8,
           linear_step = 8;
    size_t i = 0;
    *to = malloc(nmemb*sizeof(struct nline*));
    (*to)[n_to] = mkblank();
    n_to++;
    while (i < n_from) {
        if (from[i]->type == TEXT) {
                char *head = from[i]->text->data;
                size_t head_len = from[i]->text->len;
                unsigned prepend_space;
                struct string *s = malloc(sizeof(struct string));
                int found_non_whitespace = 0;
                for (prepend_space=0;
                        prepend_space<head_len && !isalpha(head[prepend_space]);
                        prepend_space++) {
                    if (!found_non_whitespace && !isspace(head[prepend_space])) {
                        found_non_whitespace = 1;
                    }
                }
                if (!found_non_whitespace || prepend_space == head_len) {
                    prepend_space = 0;
                }
                char *tail = head;
                int split_index, prev_split_index;
                while (tail != NULL) {
                    tail = index(head, ' ');
                    split_index = tail-head;
                    prev_split_index = split_index;
                    while (tail != NULL && split_index+prepend_space < width) {
                        prev_split_index = split_index;
                        tail = index(tail+1, ' ');
                        split_index = tail-head;
                    }
                    if (tail != NULL || strlen(head)+prepend_space > width) {
                        struct string *s1 = malloc(sizeof(struct string));
                        head[prev_split_index] = '\0';
                        if (head == from[i]->text->data) {
                            s1->len = strlen(head);
                            s1->data = strcpy(malloc((s1->len+1)*sizeof(char)), head);
                        } else {
                            s1->len = strlen(head)+prepend_space;
                            s1->data = malloc((s1->len+1)*sizeof(char));
                            for (int i=0; i<prepend_space; i++) {
                                s1->data[i] = ' ';
                            }
                            strcpy(s1->data+prepend_space, head);
                        }
                        struct nline *nline = malloc(sizeof(struct nline));
                        nline->type = TEXT;
                        nline->text = s1;
                        if (n_to >= nmemb) {
                            nmemb += linear_step;
                            *to = realloc(*to, nmemb*sizeof(struct nline*));
                        }
                        (*to)[n_to] = nline;
                        n_to++;
                        head[prev_split_index] = ' ';
                        head += prev_split_index+1;
                    }
                }
                if (head == from[i]->text->data) {
                    s->len = strlen(head);
                    s->data = strcpy(malloc((s->len+1)*sizeof(char)), head);
                } else {
                    s->len = strlen(head)+prepend_space;
                    s->data = malloc((s->len+1)*sizeof(char));
                    for (int i=0; i<prepend_space; i++) {
                        s->data[i] = ' ';
                    }
                    strcpy(s->data+prepend_space, head);
                }
                struct nline *nline = malloc(sizeof(struct nline));
                nline->type = TEXT;
                nline->text = s;
                if (n_to >= nmemb) {
                    nmemb += linear_step;
                    *to = realloc(*to, nmemb*sizeof(struct nline*));
                }
                (*to)[n_to] = nline;
                n_to++;
        } else if (from[i]->type == ANIM) {
            struct nline *nline = malloc(sizeof(struct nline));
            nline->type = ANIM;
            nline->anim = malloc(sizeof(struct animation));
            nline->anim->current_frame_index = from[i]->anim->current_frame_index;
            nline->anim->n_frames = from[i]->anim->n_frames;
            nline->anim->loop = from[i]->anim->loop;
            nline->anim->is_first_line = from[i]->anim->is_first_line;
            nline->anim->frames = malloc(nline->anim->n_frames*sizeof(struct frame));
            for (size_t i_frame = 0; i_frame < nline->anim->n_frames; i_frame++) {
                nline->anim->frames[i_frame].delay = from[i]->anim->frames[i_frame].delay;
                nline->anim->frames[i_frame].s = malloc(sizeof(struct string));
                nline->anim->frames[i_frame].s->len = from[i]->anim->frames[i_frame].s->len;
                nline->anim->frames[i_frame].s->data =
                    strncpy(malloc(nline->anim->frames[i_frame].s->len*sizeof(char)+1),
                            from[i]->anim->frames[i_frame].s->data,
                            from[i]->anim->frames[i_frame].s->len+1);
            }
            if (n_to >= nmemb) {
                nmemb += linear_step;
                *to = realloc(*to, nmemb*sizeof(struct nline*));
            }
            (*to)[n_to] = nline;
            n_to++;
        }
        i++;
    }
    if (!(options & NO_TRAILING_NEWLINE)) {
        if (n_to >= nmemb) {
            nmemb++;
            *to = realloc(*to, nmemb*sizeof(struct nline*));
        }
        (*to)[n_to] = mkblank();
        n_to++;
    }
    return n_to;
}

size_t gen_index(struct section **sections, size_t nmemb, struct nline ***to, size_t width) {
    *to = malloc((nmemb+2)*sizeof(struct nline*));
    (*to)[0] = mkblank();
    for (size_t i = 0; i < nmemb; i++) {
        struct string *s = malloc(sizeof(struct string));
        s->data = malloc((width+1)*sizeof(char));
        s->len = width;
        size_t j = 0;
        size_t title_len = strlen(sections[i]->title);
        while (j < width-title_len) {
            s->data[j] = ' ';
            j++;
        }
        strncpy(s->data+j, sections[i]->title, width-j);
        s->data[s->len] = '\0';
        struct nline *nline = malloc(sizeof(struct nline));
        nline->type = TEXT;
        nline->text = s;
        (*to)[i+1] = nline;
    }
    (*to)[nmemb+1] = mkblank();
    return nmemb+2;
}

void free_nlines(struct nline **nlines, size_t nmemb) {
    if (nlines != NULL) {
        for (size_t i = 0; i < nmemb; i++) {
            if (nlines[i]->type == TEXT) {
                free(nlines[i]->text->data);
                free(nlines[i]->text);
            } else if (nlines[i]->type == ANIM) {
                for (size_t j = 0; j < nlines[i]->anim->n_frames; j++) {
                    free(nlines[i]->anim->frames[j].s->data);
                    free(nlines[i]->anim->frames[j].s);
                }
                free(nlines[i]->anim->frames);
                free(nlines[i]->anim);
            }
            free(nlines[i]);
        }
        free(nlines);
    }
}

void free_content(struct window *window) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        if (window->content.lines->raw != NULL) {
            free_nlines(window->content.lines->raw, window->content.lines->n_raw);
            window->content.lines->raw = NULL;
        }
        if (window->content.lines->formatted != NULL) {
            free_nlines(window->content.lines->formatted, window->content.lines->n_formatted);
            window->content.lines->formatted = NULL;
        }
        free(window->content.lines);
        window->content.lines = NULL;
    }
    if (window->content.type == DYNAMIC && window->content.dlib != NULL) {
        if (window->content.dlib->init_fun != NULL) {
            window->content.dlib->init_fun = NULL;
        }
        if (window->content.dlib->getch_fun != NULL) {
            window->content.dlib->getch_fun = NULL;
        }
        if (window->content.dlib->scroll_fun != NULL) {
            window->content.dlib->scroll_fun = NULL;
        }
        if (window->content.dlib->kill_fun != NULL) {
            window->content.dlib->kill_fun = NULL;
        }
        if (window->content.dlib->so_handle != NULL) {
            dlclose(window->content.dlib->so_handle);
            window->content.dlib->so_handle = NULL;
        }
        free(window->content.dlib);
        window->content.dlib = NULL;
    }
}

/*
 * diagnostics functions
 */

void print_nlines(struct nline **nlines, size_t nmemb) {
    fprintf(stderr, "nlines[%u]:\n", nmemb);
    for (size_t i=0; i<nmemb; i++) {
        if (nlines[i]->type == ANIM) {
            fprintf(stderr, "  - type: ANIM\n");
            fprintf(stderr, "    anim:\n");
            fprintf(stderr, "      loop: %d\n", nlines[i]->anim->loop);
            fprintf(stderr, "      is_first_line: %d\n", nlines[i]->anim->is_first_line);
            fprintf(stderr, "      n_frames: %u\n", nlines[i]->anim->n_frames);
            fprintf(stderr, "      nmemb_frames: %u\n", nlines[i]->anim->nmemb_frames);
            fprintf(stderr, "      current_frame_index: %u\n", nlines[i]->anim->current_frame_index);
            fprintf(stderr, "      frames:\n");
            for (size_t j=0; j<nlines[i]->anim->n_frames; j++) {
                fprintf(stderr, "        - delay: %u\n", nlines[i]->anim->frames[j].delay);
                fprintf(stderr, "          s:\n");
                fprintf(stderr, "            data: %s\n", nlines[i]->anim->frames[j].s->data);
                fprintf(stderr, "            len: %u\n", nlines[i]->anim->frames[j].s->len);
            }
        } else {
            fprintf(stderr, "  - type: TEXT\n");
            fprintf(stderr, "    text:\n");
            fprintf(stderr, "      data: %s\n", nlines[i]->text->data);
            fprintf(stderr, "      len: %u\n", nlines[i]->text->len);
        }
    }
}
