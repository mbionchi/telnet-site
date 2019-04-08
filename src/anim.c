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

#include "anim.h"
#include "data.h"
#include "render.h"

#include <stdlib.h>
#include <stddef.h>

void free_anim_refs(struct window *window) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        struct anim_ref *iter = window->content.lines->anim_refs;
        while (iter != NULL) {
            struct anim_ref *tmp = iter;
            iter = iter->next;
            free(tmp);
        }
        window->content.lines->anim_refs = NULL;
    }
}

void pop_anim_ref_front(struct window *window) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        struct anim_ref *old = window->content.lines->anim_refs;
        if (old != NULL) {
            window->content.lines->anim_refs = window->content.lines->anim_refs->next;
            free(old);
        }
    }
}

void pop_anim_ref_back(struct window *window) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        struct anim_ref *iter = window->content.lines->anim_refs,
                        *prev = NULL;
        if (iter != NULL) {
            prev = iter;
            while (iter->next != NULL) {
                prev = iter;
                iter = iter->next;
            }
            prev->next = NULL;
            free(iter);
        }
    }
}

void push_anim_ref_front(struct window *window, size_t index) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        struct anim_ref *ref = malloc(sizeof(struct anim_ref));
        ref->index = index;
        ref->counter = 1;
        ref->running = 1;
        ref->next = window->content.lines->anim_refs;
        window->content.lines->anim_refs = ref;
    }
}

void push_anim_ref_back(struct window *window, size_t index) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        struct anim_ref *ref = malloc(sizeof(struct anim_ref));
        ref->index = index;
        ref->counter = 1;
        ref->running = 1;
        ref->next = NULL;
        if (window->content.lines->anim_refs == NULL) {
            window->content.lines->anim_refs = ref;
        } else {
            struct anim_ref *iter = window->content.lines->anim_refs;
            while (iter->next != NULL) {
                iter = iter->next;
            }
            iter->next = ref;
        }
    }
}

void anim_tick(struct window *window) {
    if (window->content.type == STATIC && window->content.lines != NULL) {
        struct anim_ref *iter = window->content.lines->anim_refs;
        while (iter != NULL) {
            if (iter->counter >= window->content.lines->formatted[iter->index]->anim->frames[window->content.lines->formatted[iter->index]->anim->current_frame_index].delay) {
                if (iter->running) {
                    size_t i = iter->index;
                    size_t y = i-window->scroll;
                    if (window->content.lines->formatted[i]->anim->current_frame_index ==
                            window->content.lines->formatted[i]->anim->n_frames-1 &&
                            !window->content.lines->formatted[i]->anim->loop) {
                        iter->running = 0;
                    } else {
                        scrollok(window->window, 0);
                        window->content.lines->formatted[i]->anim->current_frame_index =
                            (window->content.lines->formatted[i]->anim->current_frame_index + 1) %
                            window->content.lines->formatted[i]->anim->n_frames;
                        render_nline(window, y, window->content.lines->formatted[i]);
                        i++;
                        y++;
                        while (i < window->content.lines->n_formatted &&
                                window->content.lines->formatted[i]->type == ANIM &&
                                !window->content.lines->formatted[i]->anim->is_first_line) {
                            window->content.lines->formatted[i]->anim->current_frame_index =
                                (window->content.lines->formatted[i]->anim->current_frame_index + 1) %
                                window->content.lines->formatted[i]->anim->n_frames;
                            render_nline(window, y, window->content.lines->formatted[i]);
                            i++;
                            y++;
                        }
                        scrollok(window->window, 1);
                        wrefresh(window->window);
                        iter->counter = 1;
                    }
                }
            } else {
                iter->counter++;
            }
            iter = iter->next;
        }
    }
}
