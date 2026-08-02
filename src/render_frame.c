#include "render_internal.h"
#include "siecs.h"
#include <stdlib.h>

void sirender_begin_frame(ecs_iter_t *it) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;

    frame->cmd = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
}

void sirender_end_frame(ecs_iter_t *it) {
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;

    if (!frame->cmd)
        return;

    SDL_SubmitGPUCommandBuffer(frame->cmd);
    frame->cmd = NULL;
}

void sirender_frame_shutdown() {
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;

    frame->cmd = NULL;
}
