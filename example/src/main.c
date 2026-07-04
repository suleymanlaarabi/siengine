#include "siecs.h"
#include "siengine.h"
#include <example.h>
#include <string.h>

static ecs_entity_t make_cube(
    ecs_world_t *world,
    SIPosition3d position,
    SIRotation3d rotation,
    SIScale3d scale,
    SIColor color
) {
    ecs_entity_t cube = ecs_new(world);
    ecs_add(world, cube, SICube);
    ecs_set_cid(world, cube, ecs_id(SIPosition3d), &position);
    ecs_set_cid(world, cube, ecs_id(SIRotation3d), &rotation);
    ecs_set_cid(world, cube, ecs_id(SIScale3d), &scale);
    ecs_set_cid(world, cube, ecs_id(SIColor), &color);
    return cube;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ecs_world_t *world = ecs_init();
    ECS_MODULE_IMPORT(world, siengine, {});

    ecs_entity_t window = ecs_new(world);
    ecs_set(
        world,
        window,
        SIWindow,
        { .title = strdup("siengine cubes"),
          .width = 1280,
          .height = 720,
          .resizable = true,
          .vsync = true }
    );

    ecs_entity_t camera = ecs_new(world);
    ecs_set(world, camera, SIPosition3d, { .x = 0.0f, .y = 1.5f, .z = -6.0f });
    ecs_set(world, camera, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(
        world,
        camera,
        SICamera3d,
        { .fov_y = 1.0471976f, .near_clip = 0.1f, .far_clip = 100.0f }
    );
    ecs_add(world, camera, SIActiveCamera);

    make_cube(
        world,
        (SIPosition3d){ .x = -1.4f, .y = 0.0f, .z = 2.5f },
        (SIRotation3d){ .x = 0.2f, .y = 0.3f, .z = 0.0f },
        (SIScale3d){ .x = 1.0f, .y = 1.0f, .z = 1.0f },
        (SIColor){ .r = 0.8f, .g = 0.2f, .b = 0.15f, .a = 1.0f }
    );
    make_cube(
        world,
        (SIPosition3d){ .x = 0.0f, .y = 0.0f, .z = 3.0f },
        (SIRotation3d){ .x = 0.0f, .y = 0.5f, .z = 0.2f },
        (SIScale3d){ .x = 0.8f, .y = 1.4f, .z = 0.8f },
        (SIColor){ .r = 0.2f, .g = 0.65f, .b = 0.95f, .a = 1.0f }
    );
    make_cube(
        world,
        (SIPosition3d){ .x = 1.4f, .y = 0.0f, .z = 2.5f },
        (SIRotation3d){ .x = -0.3f, .y = -0.4f, .z = 0.1f },
        (SIScale3d){ .x = 1.0f, .y = 0.7f, .z = 1.2f },
        (SIColor){ .r = 0.25f, .g = 0.9f, .b = 0.35f, .a = 1.0f }
    );

    while (ecs_progress(world)) {
    }

    ecs_fini(world);
    return 0;
}
