#include "render_internal.h"

static void render_state_on_remove(const void *ptr) {
    SIRenderState *render = (SIRenderState *)ptr;
    sibackend_shutdown();
    sicore_vec_fini(&render->views);
    sicore_vec_fini(&render->batches);
    sicore_vec_fini(&render->instances);
}

ECS_RESOURCE_DEFINE(SIRenderState, .on_remove = render_state_on_remove);

void sirender_register(void) {
    ECS_RESOURCE_REGISTER(SIRenderState);
    ecs_set_resource(SIRenderState, {});
    SIRenderState *render = ecs_get_resource(SIRenderState);
    sicore_vec_init(&render->views, sizeof(SIRenderView));
    sicore_vec_init(&render->batches, sizeof(SIRenderBatch));
    sicore_vec_init(&render->instances, sizeof(SIInstance2D));
    sibackend_init();

    ecs_system_id_t begin_frame = ecs_system(
        {
            .name = "BeginFrame",
            .phase = EcsPreRender,
            .callback = sirender_begin_frame,
            .main_thread_only = true,
            .query = {
                .resources = {
                    ecs_in(SIEngineCtx),
                    ecs_inout(SIRenderState),
                },
            },
        }
    );
    ecs_system_id_t extract_cameras = ecs_system(
        {
            .name = "ExtractCameras",
            .phase = EcsPreRender,
            .callback = sirender_extract_cameras,
            .main_thread_only = true,
            .after = { begin_frame },
            .query = {
                .components = {
                    ecs_in(SICamera2D),
                    ecs_in(SIWorldTransform2D),
                    ecs_in(SICameraViewport),
                    ecs_in_optional(SIVirtualResolution),
                },
                .resources = {
                    ecs_inout(SIRenderState),
                },
            },
        }
    );
    ecs_system(
        {
            .name = "ExtractRenderables",
            .phase = EcsPreRender,
            .callback = sirender_extract_renderables,
            .main_thread_only = true,
            .after = { extract_cameras },
            .query = {
                .components = {
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
                .resources = {
                    ecs_inout(SIRenderState),
                },
            },
        }
    );
    ecs_system(
        {
            .name = "DrawWindow",
            .phase = EcsOnRender,
            .callback = sirender_draw_window,
            .main_thread_only = true,
            .query = {
                .resources = {
                    ecs_in(SIRenderState),
                    ecs_in(SIEngineCtx),
                },
            },
        }
    );
    ecs_system(
        {
            .name = "EndFrame",
            .phase = EcsPostRender,
            .callback = sirender_end_frame,
            .main_thread_only = true,
            .query = {
                .resources = {
                    ecs_in(SIEngineCtx),
                },
            },
        }
    );
}
