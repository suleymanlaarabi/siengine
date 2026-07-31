#include "render_internal.h"

ECS_RESOURCE_DEFINE(SIRenderState);

void sirender_shutdown() {
    sicube_render_state_shutdown();
    siwindow_render_state_shutdown();
    sirender_frame_shutdown();
}

void sirender_register() {
    ECS_RESOURCE_REGISTER(SIRenderState);
    ecs_set_resource(SIRenderState, {});

    ecs_system_id_t begin = ecs_system(
        {
            .name = "BeginFrame",
            .phase = EcsPreRender,
            .callback = sirender_begin_frame,
        }
    );
    ecs_system(
        {
            .name = "ExtractRenderViews",
            .query.terms = {
                ecs_in(SICamera3d),
                ecs_in(SIPosition3d),
                ecs_in(SIRotation3d),
                ecs_filter(SIActiveCamera),
            },
            .phase = EcsPreRender,
            .callback = sirender_extract_views,
            .after = { begin },
        }
    );
    ecs_system_id_t extract_cubes = ecs_system(
        {
            .name = "ExtractCubeInstances",
            .query.terms = {
                ecs_in(SIPosition3d),
                ecs_in(SIRotation3d),
                ecs_in(SIScale3d),
                ecs_in(SIColor),
                ecs_filter(SICube),
            },
            .phase = EcsPreRender,
            .callback = sirender_extract_cube_instances,
            .after = { begin },
        }
    );
    ecs_system(
        {
            .name = "UploadCubeInstances",
            .phase = EcsPreRender,
            .callback = sirender_upload_cube_instances,
            .after = { extract_cubes },
        }
    );
    ecs_system(
        {
            .name = "DrawWindow",
            .phase = EcsOnRender,
            .callback = sirender_draw_window,
        }
    );
    ecs_system(
        {
            .name = "EndFrame",
            .phase = EcsPostRender,
            .callback = sirender_end_frame,
        }
    );
}
