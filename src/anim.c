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

#include <stdlib.h>
#include <stddef.h>

void free_anim_refs(struct window *window) {
    struct anim_ref *iter = window->content.anim_refs;
    while (iter != NULL) {
        struct anim_ref *tmp = iter;
        iter = iter->next;
        free(tmp);
    }
    window->content.anim_refs = NULL;
}

void pop_anim_ref_front(struct window *window) {
    struct anim_ref *old = window->content.anim_refs;
    if (old != NULL) {
        window->content.anim_refs = window->content.anim_refs->next;
        free(old);
    }
}

void pop_anim_ref_back(struct window *window) {
    struct anim_ref *iter = window->content.anim_refs,
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

void push_anim_ref_front(struct window *window, size_t index) {
    struct anim_ref *ref = malloc(sizeof(struct anim_ref));
    ref->index = index;
    ref->counter = 1;
    ref->running = 1;
    ref->next = window->content.anim_refs;
    window->content.anim_refs = ref;
}

void push_anim_ref_back(struct window *window, size_t index) {
    struct anim_ref *ref = malloc(sizeof(struct anim_ref));
    ref->index = index;
    ref->counter = 1;
    ref->running = 1;
    ref->next = NULL;
    if (window->content.anim_refs == NULL) {
        window->content.anim_refs = ref;
    } else {
        struct anim_ref *iter = window->content.anim_refs;
        while (iter->next != NULL) {
            iter = iter->next;
        }
        iter->next = ref;
    }
}

void anim_tick(struct window *window) {
    struct anim_ref *iter = window->content.anim_refs;
    while (iter != NULL) {
        if (iter->counter >= window->content.formatted[iter->index]->anim->frames[window->content.formatted[iter->index]->anim->current_frame_index].delay) {
            if (iter->running) {
                size_t i = iter->index;
                size_t y = i-window->scroll;
                if (window->content.formatted[i]->anim->current_frame_index ==
                        window->content.formatted[i]->anim->n_frames-1 &&
                        !window->content.formatted[i]->anim->loop) {
                    iter->running = 0;
                } else {
                    scrollok(window->window, 0);
                    window->content.formatted[i]->anim->current_frame_index =
                        (window->content.formatted[i]->anim->current_frame_index + 1) %
                        window->content.formatted[i]->anim->n_frames;
                    mvwprintw(window->window, y, 0, "%s",
                            window->content.formatted[i]->anim->frames[window->content.formatted[i]->anim->current_frame_index].s->data);
                    i++;
                    y++;
                    while (i < window->content.n_formatted &&
                            window->content.formatted[i]->type == ANIM &&
                            !window->content.formatted[i]->anim->is_first_line) {
                        window->content.formatted[i]->anim->current_frame_index =
                            (window->content.formatted[i]->anim->current_frame_index + 1) %
                            window->content.formatted[i]->anim->n_frames;
                        mvwprintw(window->window, y, 0, "%s",
                                window->content.formatted[i]->anim->frames[window->content.formatted[i]->anim->current_frame_index].s->data);
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
