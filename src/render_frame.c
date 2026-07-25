#include "render_internal.h"
#include <SDL3/SDL_error.h>
#include <stdio.h>
#include <stdlib.h>

void sirender_begin_frame(ecs_iter_t *it) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderFrame *frame = ecs_resource(SIRenderFrame);
    SICubeRenderState *cubes = ecs_resource(SICubeRenderState);

    frame->view_count = 0;
    cubes->instance_count = 0;
    cubes->instances_uploaded = false;

    if (engine->primary_gpu == NULL) {
        frame->cmd = NULL;
        return;
    }

    frame->cmd = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
    if (frame->cmd == NULL) {
        fprintf(stderr, "siengine: SDL_AcquireGPUCommandBuffer failed: %s\n", SDL_GetError());
    }
}

void sirender_end_frame(ecs_iter_t *it) {
    SIRenderFrame *frame = ecs_resource(SIRenderFrame);
    if (frame->cmd == NULL) {
        return;
    }

    SDL_SubmitGPUCommandBuffer(frame->cmd);
    frame->cmd = NULL;
}

void sirender_frame_shutdown() {
    SIRenderFrame *frame = ecs_try_resource(SIRenderFrame);
    if (frame == NULL) {
        return;
    }

    free(frame->views);
    frame->views = NULL;
    frame->view_count = 0;
    frame->view_capacity = 0;
    frame->cmd = NULL;
}

void sirender_queries_shutdown() {
    SIRenderQueries *queries = ecs_try_resource(SIRenderQueries);
    if (queries == NULL) {
        return;
    }

    if (queries->cameras) {
        ecs_query_fini(queries->cameras);
        queries->cameras = 0;
    }
}
