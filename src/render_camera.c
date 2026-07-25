#include "render_internal.h"
#include <stdio.h>
#include <stdlib.h>

static inline void ensure_view_capacity(SIRenderFrame *frame, uint32_t needed) {
    if (needed <= frame->view_capacity) {
        return;
    }

    uint32_t capacity = frame->view_capacity ? frame->view_capacity : 4;
    while (capacity < needed) {
        capacity *= 2;
    }

    frame->views = realloc(frame->views, sizeof(SIRenderView) * capacity);
    frame->view_capacity = capacity;
}

void sirender_extract_views(ecs_iter_t *it) {
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;
    SICamera3d *cameras = ecs_field(it, 0);
    SIPosition3d *positions = ecs_field(it, 1);
    SIRotation3d *rotations = ecs_field(it, 2);

    ensure_view_capacity(frame, frame->view_count + it->count);

    for (uint32_t i = 0; i < it->count; i++) {
        SIRenderView *view = &frame->views[frame->view_count++];
        view->camera = cameras[i];
        view->view = si_mat4_view(positions[i], rotations[i]);
    }
}
