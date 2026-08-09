#include "render_internal.h"

ECS_RESOURCE_DEFINE(SIRenderState);

void sirender_shutdown(void) {
    SIRenderState *render = ecs_resource(SIRenderState);
    ecs_query_fini(render->camera_query);
    ecs_query_fini(render->renderable_query);
    sibackend_shutdown();
    sicore_vec_fini(&render->views);
    sicore_vec_fini(&render->batches);
    sicore_vec_fini(&render->instances);
}

void sirender_register(void) {
    ECS_RESOURCE_REGISTER(SIRenderState);
    ecs_set_resource(SIRenderState, {});
    SIRenderState *render = ecs_resource(SIRenderState);
    sicore_vec_init(&render->views, sizeof(SIRenderView));
    sicore_vec_init(&render->batches, sizeof(SIRenderBatch));
    sicore_vec_init(&render->instances, sizeof(SIInstance2D));
    sibackend_init();

    render->camera_query = ecs_query({
        .terms = {
            ecs_in(SICamera2D),
            ecs_in(SIWorldTransform2D),
            ecs_in(SICameraViewport),
            ecs_in_optional(SIVirtualResolution),
        },
    });
    render->renderable_query = ecs_query({
        .terms = {
            ecs_filter(SIRenderable),
            ecs_in(SIWorldTransform2D),
            ecs_in(SIColor),
            ecs_in_optional(SISprite),
            ecs_in_optional(SISpriteFlip),
            ecs_in_optional(SICircle),
            ecs_in_optional(SIRectangle),
            ecs_in_optional(SITriangle),
            ecs_up(SIMaterial2D, Material),
            ecs_up(SISpriteSheet, Material),
            ecs_up(SIPivot, Material),
            ecs_up(SIBlendMode, Material),
        },
        .relations = {
            ecs_rel(Material),
            ecs_rel(Layer),
        },
        .order_by = ecs_order_by_target(Layer),
    });

    ecs_system({
        .name = "BeginFrame",
        .phase = EcsPreRender,
        .callback = sirender_begin_frame,
    });
    ecs_system({
        .name = "ExtractRender",
        .phase = EcsPreRender,
        .callback = sirender_extract,
    });
    ecs_system({
        .name = "DrawWindow",
        .phase = EcsOnRender,
        .callback = sirender_draw_window,
    });
    ecs_system({
        .name = "EndFrame",
        .phase = EcsPostRender,
        .callback = sirender_end_frame,
    });
}
