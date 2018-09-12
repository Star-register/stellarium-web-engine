/* Stellarium Web Engine - Copyright (c) 2018 - Noctua Software Ltd
 *
 * This program is licensed under the terms of the GNU AGPL v3, or
 * alternatively under a commercial licence.
 *
 * The terms of the AGPL v3 license can be found in the main directory of this
 * repository.
 */

#include "gesture.h"
#include "vec.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct inputs inputs_t;
struct inputs {
    struct {
        double pos[2];
        bool   down[2];
    } ts[2];
};
static inputs_t g_inputs = {};

// XXX: this value should be set depending on the screen resolution.
static double g_start_dist = 8;



static int pan_on_mouse(gesture_t *g, const inputs_t *in)
{
    vec2_set(g->pos, in->ts[0].pos[0], in->ts[0].pos[1]);
    if (g->state == GESTURE_POSSIBLE && in->ts[0].down[0]) {
        g->state = GESTURE_RECOGNISED;
        vec2_copy(g->pos, g->start_pos[0]);
    }
    if (g->state == GESTURE_RECOGNISED && in->ts[0].down[0]) {
        if (vec2_dist2(g->start_pos[0], g->pos) > g_start_dist) {
            g->state = GESTURE_BEGIN;
            g->callback(g, NULL);
            g->state = GESTURE_UPDATE;
            return 1;
        }
    }
    if (g->state == GESTURE_UPDATE && in->ts[0].down[0]) {
        g->callback(g, NULL);
        return 1;
    }
    if (g->state == GESTURE_UPDATE && !in->ts[0].down[0]) {
        g->state = GESTURE_END;
        g->callback(g, NULL);
        return 1;
    }
    return 0;
}

static int pinch_on_mouse(gesture_t *g, const inputs_t *in)
{
    switch (g->state) {
    case GESTURE_POSSIBLE:
        if (in->ts[0].down[0] && in->ts[1].down[0]) {
            if (vec2_dist(in->ts[0].pos, in->ts[1].pos) < g_start_dist)
                return 0;
            g->state = GESTURE_BEGIN;
            vec2_copy(in->ts[0].pos, g->start_pos[0]);
            vec2_copy(in->ts[1].pos, g->start_pos[1]);
            g->pinch = 1.0;
            vec2_mix(in->ts[0].pos, in->ts[1].pos, 0.5, g->pos);
            g->callback(g, NULL);
            return 1;
        }
        break;
    case GESTURE_BEGIN:
    case GESTURE_UPDATE:
        g->state = GESTURE_UPDATE;
        g->pinch = vec2_dist(in->ts[0].pos, in->ts[1].pos) /
                   vec2_dist(g->start_pos[0], g->start_pos[1]);
        vec2_mix(in->ts[0].pos, in->ts[1].pos, 0.5, g->pos);
        if (!in->ts[0].down[0] || !in->ts[1].down[0]) {
            g->state = GESTURE_END;
            g->callback(g, NULL);
            return 1;
        }
        g->callback(g, NULL);
        return 1;
        break;
    }
    return 0;
}

static int click_on_mouse(gesture_t *g, const inputs_t *in)
{
    if (g->state == GESTURE_POSSIBLE && in->ts[0].down[0]) {
        g->state = GESTURE_RECOGNISED;
    }
    if (g->state == GESTURE_RECOGNISED && !in->ts[0].down[0]) {
        vec2_set(g->pos, in->ts[0].pos[0], in->ts[0].pos[1]);
        g->state = GESTURE_BEGIN;
        g->callback(g, NULL);
        g->state = GESTURE_END;
        return 1;
    }
    return 0;
}

static int hover_on_mouse(gesture_t *g, const inputs_t *in)
{
    if (g->state == GESTURE_POSSIBLE && !in->ts[0].down[0]) {
        vec2_set(g->pos, in->ts[0].pos[0], in->ts[0].pos[1]);
        g->callback(g, NULL);
    }
    return 0;
}

int gesture_on_mouse(int n, gesture_t **gs, int id, int state,
                     double x, double y)
{
    int i, gi;
    gesture_t *g;
    g_inputs.ts[id].pos[0] = x;
    g_inputs.ts[id].pos[1] = y;
    g_inputs.ts[id].down[0] = (state == 1);

    for (i = 0; i < n; i++) {
        g = gs[i];
        if (g->type == GESTURE_PAN && pan_on_mouse(g, &g_inputs))
            break;
        if (g->type == GESTURE_PINCH && pinch_on_mouse(g, &g_inputs))
            break;
        if (g->type == GESTURE_CLICK && click_on_mouse(g, &g_inputs))
            break;
        if (g->type == GESTURE_HOVER && hover_on_mouse(g, &g_inputs))
            break;
    }
    gi = i;
    if (gi >= n) return 0;
    // If a gesture ended, we can make all the gestures possible again.
    if (gs[gi]->state == GESTURE_END) {
        for (i = 0; i < n; i++) gs[i]->state = GESTURE_POSSIBLE;
        return 0;
    }
    // If a gesture was triggered, we cancel all the others.
    for (i = 0; i < n; i++) {
        g = gs[i];
        if (i != gi) g->state = GESTURE_FAILED;
    }
    return 0;
}
