#include "siecs.h"
#include "siengine.h"
#include <example.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_set_resource(
        SIWindow,
        {
            .title = "siengine 2d",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .vsync = true,
        }
    );

    ecs_entity_t camera = ecs_new();
    ecs_set(camera, Name, { "Camera" });
    ecs_set(
        camera,
        SICamera2D,
        { .zoom = 1.0f, .viewport_width = 320.0f, .viewport_height = 180.0f }
    );
    ecs_add(camera, SIActiveCamera);

    for (int i = 0; i < 100; i++) {
        ecs_new();
    }

    while (ecs_progress()) {
    }

    ecs_fini();
    return 0;
}
