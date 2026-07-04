#include <test.h>

static void register_scene_components(ecs_world_t *world) {
    ECS_COMPONENT_REGISTER(world, SIPosition3d);
    ECS_COMPONENT_REGISTER(world, SIRotation3d);
    ECS_COMPONENT_REGISTER(world, SIScale3d);
    ECS_COMPONENT_REGISTER(world, SIColor);
    ECS_COMPONENT_REGISTER(world, SICube);
    ECS_COMPONENT_REGISTER(world, SICamera3d);
    ECS_COMPONENT_REGISTER(world, SIActiveCamera);
}

void component_cube_query_matches_transform_and_color(void) {
    ecs_world_t *world = ecs_init();
    register_scene_components(world);

    ecs_entity_t cube = ecs_new(world);
    ecs_add(world, cube, SICube);
    ecs_set(world, cube, SIPosition3d, { .x = 1.0f, .y = 2.0f, .z = 3.0f });
    ecs_set(world, cube, SIRotation3d, { .x = 0.1f, .y = 0.2f, .z = 0.3f });
    ecs_set(world, cube, SIScale3d, { .x = 2.0f, .y = 2.0f, .z = 2.0f });
    ecs_set(world, cube, SIColor, { .r = 0.2f, .g = 0.4f, .b = 0.6f, .a = 1.0f });

    ecs_query_id_t query = ecs_query(
        world,
        { .terms = {
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_in(SIScale3d),
              ecs_in(SIColor),
              ecs_filter(SICube),
          } }
    );
    ecs_iter_t it = ecs_query_iter(world, query);

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

    ecs_query_fini(world, query);
    ecs_fini(world);
}

void component_camera_query_matches_active_camera(void) {
    ecs_world_t *world = ecs_init();
    register_scene_components(world);

    ecs_entity_t inactive = ecs_new(world);
    ecs_set(world, inactive, SIPosition3d, { .x = 0.0f, .y = 0.0f, .z = -3.0f });
    ecs_set(world, inactive, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(world, inactive, SICamera3d, { .fov_y = 1.0f, .near_clip = 0.1f, .far_clip = 10.0f });

    ecs_entity_t active = ecs_new(world);
    ecs_set(world, active, SIPosition3d, { .x = 0.0f, .y = 1.0f, .z = -6.0f });
    ecs_set(world, active, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(world, active, SICamera3d, { .fov_y = 1.0f, .near_clip = 0.1f, .far_clip = 100.0f });
    ecs_add(world, active, SIActiveCamera);

    ecs_query_id_t query = ecs_query(
        world,
        { .terms = {
              ecs_in(SICamera3d),
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_filter(SIActiveCamera),
          } }
    );
    ecs_iter_t it = ecs_query_iter(world, query);

    test_true(ecs_iter_next(&it));
    test_int(1, it.count);
    test_assert(it.entities[0] == active);
    test_false(ecs_iter_next(&it));

    ecs_query_fini(world, query);
    ecs_fini(world);
}
