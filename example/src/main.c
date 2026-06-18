#include "siecs.h"
#include "siengine.h"
#include <example.h>

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();
    ECS_MODULE_IMPORT(world, siengine, {});

    ecs_entity_t window = ecs_new(world);
    ecs_set(world, window, SIWindow, {
        .ptr = siengine_create_window(world, "engine").ptr
    });

    ecs_entity_t window2 = ecs_new(world);
    ecs_set(world, window2, SIWindow, {
        .ptr = siengine_create_window(world, "engine").ptr
    });

    while (ecs_progress(world)) {}

    ecs_fini(world);
    return 0;
}
