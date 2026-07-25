#include "render_internal.h"
#include <stdio.h>
#include <stdlib.h>

static inline bool ensure_view_capacity(SIRenderFrame *frame, uint32_t needed) {
    if (needed <= frame->view_capacity) {
        return true;
    }

    uint32_t capacity = frame->view_capacity ? frame->view_capacity : 4;
    while (capacity < needed) {
        capacity *= 2;
    }

    SIRenderView *views = realloc(frame->views, sizeof(SIRenderView) * capacity);
    if (views == NULL) {
        fprintf(stderr, "siengine: failed to grow render view buffer\n");
        return false;
    }

    frame->views = views;
    frame->view_capacity = capacity;
    return true;
}

void sirender_extract_views(ecs_iter_t *it) {
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;
    SICamera3d *cameras = ecs_field(it, 0);
    SIPosition3d *positions = ecs_field(it, 1);
    SIRotation3d *rotations = ecs_field(it, 2);

    if (!ensure_view_capacity(frame, frame->view_count + it->count)) {
        return;
    }

    for (uint32_t i = 0; i < it->count; i++) {
        SIRenderView *view = &frame->views[frame->view_count++];
        view->camera = cameras[i];
        view->view = si_mat4_view(positions[i], rotations[i]);
    }
}
