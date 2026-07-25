#include "test.h"

static void register_scene_components() {

    ECS_COMPONENT_REGISTER(SIPosition3d);
    ECS_COMPONENT_REGISTER(SIRotation3d);
    ECS_COMPONENT_REGISTER(SIScale3d);
    ECS_COMPONENT_REGISTER(SIColor);
    ECS_COMPONENT_REGISTER(SICube);
    ECS_COMPONENT_REGISTER(SICamera3d);
    ECS_COMPONENT_REGISTER(SIActiveCamera);
}

void component_cube_query_matches_transform_and_color(void) {
    ecs_init();
    register_scene_components();

    ecs_entity_t cube = ecs_new();
    ecs_add(cube, SICube);
    ecs_set(cube, SIPosition3d, { .x = 1.0f, .y = 2.0f, .z = 3.0f });
    ecs_set(cube, SIRotation3d, { .x = 0.1f, .y = 0.2f, .z = 0.3f });
    ecs_set(cube, SIScale3d, { .x = 2.0f, .y = 2.0f, .z = 2.0f });
    ecs_set(cube, SIColor, { .r = 0.2f, .g = 0.4f, .b = 0.6f, .a = 1.0f });

    ecs_query_id_t query = ecs_query(
        { .terms = {
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_in(SIScale3d),
              ecs_in(SIColor),
              ecs_filter(SICube),
          }, }
    );
    ecs_iter_t it = ecs_query_iter(query);

    test_true(ecs_iter_next(&it));
    test_int(1, it.count);

    SIPosition3d *positions = ecs_field(&it, 0);
    SIScale3d *scales = ecs_field(&it, 2);
    SIColor *colors = ecs_field(&it, 3);

    test_assert(positions[0].x == 1.0f);
    test_assert(positions[0].y == 2.0f);
    test_assert(positions[0].z == 3.0f);
    test_assert(scales[0].x == 2.0f);
    test_assert(colors[0].b == 0.6f);
    test_false(ecs_iter_next(&it));

    ecs_query_fini(query);
    ecs_fini();
}

void component_camera_query_matches_active_camera(void) {
    ecs_init();
    register_scene_components();

    ecs_entity_t inactive = ecs_new();
    ecs_set(inactive, SIPosition3d, { .x = 0.0f, .y = 0.0f, .z = -3.0f });
    ecs_set(inactive, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(inactive, SICamera3d, { .fov_y = 1.0f, .near_clip = 0.1f, .far_clip = 10.0f });

    ecs_entity_t active = ecs_new();
    ecs_set(active, SIPosition3d, { .x = 0.0f, .y = 1.0f, .z = -6.0f });
    ecs_set(active, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(active, SICamera3d, { .fov_y = 1.0f, .near_clip = 0.1f, .far_clip = 100.0f });
    ecs_add(active, SIActiveCamera);

    ecs_query_id_t query = ecs_query(
        { .terms = {
              ecs_in(SICamera3d),
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_filter(SIActiveCamera),
          } }
    );
    ecs_iter_t it = ecs_query_iter(query);

    test_true(ecs_iter_next(&it));
    test_int(1, it.count);
    test_assert(it.entities[0] == active);
    test_false(ecs_iter_next(&it));

    ecs_query_fini(query);
    ecs_fini();
}

void component_camera_query_matches_multiple_active_cameras(void) {
    ecs_init();
    register_scene_components();

    ecs_entity_t first = ecs_new();
    ecs_set(first, SIPosition3d, { .x = -1.0f, .y = 1.0f, .z = -6.0f });
    ecs_set(first, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(first, SICamera3d, { .fov_y = 1.0f, .near_clip = 0.1f, .far_clip = 100.0f });
    ecs_add(first, SIActiveCamera);

    ecs_entity_t second = ecs_new();
    ecs_set(second, SIPosition3d, { .x = 1.0f, .y = 1.0f, .z = -6.0f });
    ecs_set(second, SIRotation3d, { .x = 0.0f, .y = 0.2f, .z = 0.0f });
    ecs_set(second, SICamera3d, { .fov_y = 0.8f, .near_clip = 0.1f, .far_clip = 50.0f });
    ecs_add(second, SIActiveCamera);

    ecs_query_id_t query = ecs_query(
        { .terms = {
              ecs_in(SICamera3d),
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_filter(SIActiveCamera),
          } }
    );
    ecs_iter_t it = ecs_query_iter(query);
    uint32_t count = 0;

    while (ecs_iter_next(&it)) {
        count += it.count;
    }

    test_int(2, count);

    ecs_query_fini(query);
    ecs_fini();
}
