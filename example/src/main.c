#include "siecs.h"
#include "siengine.h"
#include <example.h>
#include <string.h>

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();
    ECS_MODULE_IMPORT(world, siengine, {});

    ecs_entity_t window = ecs_new(world);
    ecs_set(world, window, SIWindow, { .title = strdup("window 2") });

    ecs_entity_t window2 = ecs_new(world);
    ecs_set(world, window2, SIWindow, { .title = strdup("window 1") });

    ecs_set(world, window2, SIWindow, { .title = strdup("coucou") });

    // ecs_entity_t character = ecs_new(world);
    // ecs_set(world, character, Position, { 0, 0 });
    // ecs_set(world, character, Image, { "character.png" });
    // ecs_set(world, player, Sprite, {
    //     .texture_grid = {
    //         .cell_width = 100,
    //         .cell_height = 100,
    //         .rows = 8,
    //         .cols = 8,
    //     },
    //     .sprite = 0,
    // });

    ecs_entity_t player = ecs_new(world);
    // ecs_is_a(world, player, character);

    while (ecs_progress(world)) {
    }

    ecs_fini(world);
    return 0;
}
