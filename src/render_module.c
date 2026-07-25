#include "render_internal.h"

ECS_RESOURCE_DEFINE(SIRenderQueries);
ECS_RESOURCE_DEFINE(SIRenderFrame);
ECS_RESOURCE_DEFINE(SICubeRenderState);
ECS_RESOURCE_DEFINE(SIWindowRenderState);

void sirender_register_resources() {
    ECS_RESOURCE_REGISTER(SIRenderQueries);
    ECS_RESOURCE_REGISTER(SIRenderFrame);
    ECS_RESOURCE_REGISTER(SICubeRenderState);
    ECS_RESOURCE_REGISTER(SIWindowRenderState);

    ecs_query_id_t cameras = ecs_query(
        { .terms = {
              ecs_in(SICamera3d),
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_filter(SIActiveCamera),
          } }
    );

    ecs_set_resource(SIRenderQueries, { .cameras = cameras });
    ecs_set_resource(SIRenderFrame, {});
    ecs_set_resource(SICubeRenderState, {});
    ecs_set_resource(SIWindowRenderState, {});
}

void sirender_shutdown() {
    sicube_render_state_shutdown();
    siwindow_render_state_shutdown();
    sirender_frame_shutdown();
    sirender_queries_shutdown();
}

void sirender_register() {
    sirender_register_resources();

    ecs_system_id_t begin = ecs_system(
        {
            .name = "BeginFrame",
            .phase = EcsPreRender,
            .callback = sirender_begin_frame,
        }
    );
    ecs_system_id_t extract_views = ecs_system(
        {
            .name = "ExtractRenderViews",
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
            .name = "DrawWindows",
            .query.terms = { ecs_inout(SIWindowHandle) },
            .phase = EcsOnRender,
            .callback = sirender_draw_windows,
        }
    );
    ecs_system(
        {
            .name = "EndFrame",
            .phase = EcsPostRender,
            .callback = sirender_end_frame,
        }
    );

    (void)extract_views;
}
