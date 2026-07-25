#include "render_internal.h"
#include <stdlib.h>

void sirender_begin_frame(ecs_iter_t *it) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderFrame *frame = ecs_resource(SIRenderFrame);
    SICubeRenderState *cubes = ecs_resource(SICubeRenderState);

    frame->view_count = 0;
    cubes->instance_count = 0;
    cubes->instances_uploaded = false;

    frame->cmd = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
}

void sirender_end_frame(ecs_iter_t *it) {
    SIRenderFrame *frame = ecs_resource(SIRenderFrame);
    SDL_SubmitGPUCommandBuffer(frame->cmd);
    frame->cmd = NULL;
}

void sirender_frame_shutdown() {
    SIRenderFrame *frame = ecs_resource(SIRenderFrame);

    free(frame->views);
    frame->views = NULL;
    frame->view_count = 0;
    frame->view_capacity = 0;
    frame->cmd = NULL;
}

void sirender_queries_shutdown() {
    SIRenderQueries *queries = ecs_resource(SIRenderQueries);
    ecs_query_fini(queries->cameras);
    queries->cameras = 0;
}
