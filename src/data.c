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

#include "data.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

void print_lines(struct line *lines) {
    struct line *iter = lines;
    while (iter) {
        fprintf(stderr, "%s\n", iter->line);
        iter = iter->next;
    }
}

void dump_sections(struct section **sections, size_t n_sections) {
    fprintf(stderr, "%u sections total\n", n_sections);
    for (int i=0; i<n_sections; i++) {
        fprintf(stderr, "%s (%s):\n\n", sections[i]->title, sections[i]->filename);
        print_lines(sections[i]->content);
        fprintf(stderr, "\n");
    }
}

int compare_sections(const void *a, const void *b) {
    struct section *s1 = *(struct section**)a;
    struct section *s2 = *(struct section**)b;
    return strcmp(s1->filename, s2->filename);
}

/* TODO: errcheck */
struct section **read_sections(DIR *dir, char *dirname, int follow_links, size_t *nmemb) {
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
            if (follow_links) {
                stat(filename, &sb);
            } else {
                lstat(filename, &sb);
            }
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
                base[i]->title = strcpy(malloc((title_len+1)*sizeof(char)), title);
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

void free_lines(struct line *lines) {
    struct line *iter = lines;
    struct line *prev;
    while (iter) {
        prev = iter;
        iter = iter->next;
        free(prev->line);
        free(prev);
    }
}

/*
 * this flow algorithm has:
 *   - word wrapping
 *   - list indentation, for example
 *     like this <-
 *   - a bug that will SIGSEGV this
 *     program if a contigous word is
 *     longer than `width`
 *   - it also adds a leading and
 *     trailing \n for style purposes
 */
struct line *flow_content(struct line *lines, int width) {
    struct line *rv_head = malloc(sizeof(struct line));
    struct line *rv_iter = rv_head;
    struct line *rv_prev = NULL;
    rv_iter->prev = NULL;
    rv_iter->line = strcpy(malloc(sizeof(char)), ""); /* this may seem stupid but is needed for
                                                         consistency when using free() */
    rv_iter->next = malloc(sizeof(struct line));
    rv_prev = rv_iter;
    rv_iter = rv_iter->next;
    rv_iter->prev = rv_prev;
    struct line *iter = lines;
    while (iter) {
        char *head = iter->line;
        size_t head_len = strlen(head);
        unsigned prepend_space;
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
                head[prev_split_index] = '\0';
                if (head == iter->line) {
                    rv_iter->line = strcpy(malloc((strlen(head)+1)*sizeof(char)), head);
                } else {
                    rv_iter->line = malloc((strlen(head)+prepend_space+1)*sizeof(char));
                    for (int i=0; i<prepend_space; i++) {
                        rv_iter->line[i] = ' ';
                    }
                    strcpy(rv_iter->line+prepend_space, head);
                }
                rv_iter->next = malloc(sizeof(struct line));
                rv_prev = rv_iter;
                rv_iter = rv_iter->next;
                rv_iter->prev = rv_prev;
                head[prev_split_index] = ' ';
                head += prev_split_index+1;
            }
        }
        if (head == iter->line) {
            rv_iter->line = strcpy(malloc((strlen(head)+1)*sizeof(char)), head);
        } else {
            rv_iter->line = malloc((strlen(head)+prepend_space+1)*sizeof(char));
            for (int i=0; i<prepend_space; i++) {
                rv_iter->line[i] = ' ';
            }
            strcpy(rv_iter->line+prepend_space, head);
        }
        rv_iter->next = malloc(sizeof(struct line));
        rv_prev = rv_iter;
        rv_iter = rv_iter->next;
        rv_iter->prev = rv_prev;
        iter = iter->next;
    }
    rv_iter->line = strcpy(malloc(sizeof(char)), "");
    rv_iter->next = NULL;
    return rv_head;
}

// ======== new stuff:
// big todo: errcheck mallocs, strncpys, getlines

struct nline *mkblank() {
    struct nline *blank = malloc(sizeof(struct nline));
    blank->type = TEXT;
    blank->text = malloc(sizeof(struct string));
    blank->text->len = 0;
    blank->text->data = strcpy(malloc(1*sizeof(char)), "");
    return blank;
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

size_t flow_nlines(struct nline **from, size_t n_from, struct nline ***to, int width) {
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
    if (n_to >= nmemb) {
        nmemb++;
        *to = realloc(*to, nmemb*sizeof(struct nline*));
    }
    (*to)[n_to] = mkblank();
    n_to++;
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
    if (window->content.raw != NULL) {
        free_nlines(window->content.raw, window->content.n_raw);
        window->content.raw = NULL;
    }
    if (window->content.formatted != NULL) {
        free_nlines(window->content.formatted, window->content.n_formatted);
        window->content.formatted = NULL;
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
