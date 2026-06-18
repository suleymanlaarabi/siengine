#include "siecs.h"
#include "siengine.h"
#include <example.h>

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();
    ECS_MODULE_IMPORT(world, siengine, {});

    ecs_entity_t window = ecs_new(world);
    ecs_set(world, window, SIWindow, {
        .handle = siengine_create_window(world, "engine").handle
    });

    while (ecs_progress(world)) {}

    ecs_fini(world);
    return 0;
}
