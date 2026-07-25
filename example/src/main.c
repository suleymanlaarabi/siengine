#include "siecs.h"
#include "siengine.h"
#include <example.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#define DEG2RAD(deg) ((deg) * 0.01745329251994329576923690768489)

void rotate_cube(ecs_iter_t *it) {
    SIRotation3d *rotations = ecs_field(it, 0);

    for (uint32_t i = 0; i < it->count; i++) {
        rotations[i].x += DEG2RAD(50) * it->delta_time;
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_entity_t window = ecs_new();
    ecs_set(
        window,
        SIWindow,
        { .title = "siengine cubes",
          .width = 1280,
          .height = 720,
          .resizable = true,
          .vsync = true }
    );

    ecs_entity_t camera = ecs_new();
    ecs_set(camera, SIPosition3d, { .x = 0.0f, .y = 1.5f, .z = -6.0f });
    ecs_set(camera, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(camera, SICamera3d, { .fov_y = 1.0471976f, .near_clip = 0.1f, .far_clip = 100.0f });
    ecs_add(camera, SIActiveCamera);

    ecs_entity_t cube = ecs_new();
    ecs_add(cube, SICube);
    ecs_add(cube, Abstract);

    ecs_entity_t cube1 = ecs_new();
    ecs_is_a(cube1, cube);
    ecs_set(cube1, SIPosition3d, { -1.4f, 0.0f, 2.5f });
    ecs_set(cube1, SIRotation3d, { 0.2f, 0.3f, 0.0f });
    ecs_set(cube1, SIScale3d, { 1.0f, 1.0f, 1.0f });
    ecs_set(cube1, SIColor, { 0.8f, 0.2f, 0.15f, 1.0f });

    ecs_entity_t cube2 = ecs_new();
    ecs_is_a(cube2, cube);
    ecs_set(cube2, SIPosition3d, { 0.0f, 0.0f, 3.0f });
    ecs_set(cube2, SIRotation3d, { 0.0f, 0.5f, 0.2f });
    ecs_set(cube2, SIScale3d, { 0.8f, 1.4f, 0.8f });
    ecs_set(cube2, SIColor, { 0.2f, 0.65f, 0.95f, 1.0f });

    ecs_entity_t cube3 = ecs_new();
    ecs_is_a(cube3, cube);
    ecs_set(cube3, SIPosition3d, { 1.4f, 0.0f, 2.5f });
    ecs_set(cube3, SIRotation3d, { -0.3f, -0.4f, 0.1f });
    ecs_set(cube3, SIScale3d, { 1.0f, 0.7f, 1.2f });
    ecs_set(cube3, SIColor, { 0.25f, 0.9f, 0.35f, 1.0f });

    ecs_system(
        {
            .phase = EcsOnUpdate,
            .query.terms = { ecs_inout(SIRotation3d), ecs_filter(SICube) },
            .callback = rotate_cube,
        }
    );

    while (ecs_progress()) {
    }

    ecs_fini();
    return 0;
}
