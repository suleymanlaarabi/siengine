#include "siecs.h"
#include "siengine.h"
#include <example.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    ecs_with_features({ .target_fps = 60 });
    ECS_MODULE_IMPORT(siengine, {});
    ECS_MODULE_IMPORT(SiecsRest, {});

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

    const char *hero_path =
        access("assets/hero.png", R_OK) == 0 ? "assets/hero.png" : "../assets/hero.png";
    SITextureHandle texture = siengine_load_texture(hero_path, SI_FILTER_NEAREST);

    ecs_entity_t camera = ecs_new();
    ecs_set(
        camera,
        SICamera2D,
        { .zoom = 1.0f, .viewport_width = 320.0f, .viewport_height = 180.0f }
    );
    ecs_set(camera, Name, { strdup("Camera") });
    ecs_add(camera, SIActiveCamera);
    ecs_set(camera, SICameraViewport, { .width = 1.0f, .height = 1.0f });
    ecs_set(
        camera,
        SIVirtualResolution,
        {
            .width = 320,
            .height = 180,
            .enabled = true,
            .pixel_perfect = true,
        }
    );

    ecs_entity_t root = ecs_new();
    ecs_set(root, Name, { strdup("Sprites") });

    for (int i = 0; i < 120; i++) {
        ecs_entity_t sprite = ecs_new();
        ecs_relate(sprite, ChildOf, root);
        ecs_set(
            sprite,
            SITransform2D,
            {
                .x = i % 12 * 26.0f,
                .y = (float)i / 12 * 18.0f,
                .scale_x = 0.5f,
                .scale_y = 0.5f,
            }
        );
        ecs_set(sprite, SISprite, { .texture = texture, .frame_index = 0 });
        ecs_set(
            sprite,
            SISpriteSheet,
            {
                .columns = 4,
                .rows = 1,
                .frame_width = 320,
                .frame_height = 256,
            }
        );
        ecs_set(sprite, SIColor, { .r = 1, .g = 1, .b = 1, .a = 1 });
        ecs_relate(sprite, Layer, i % 3 ? SILayerActors : SILayerEffects);
        if (i == 60) {
            ecs_set(
                sprite,
                SIAnimation,
                {
                    .start_index = 0,
                    .end_index = 3,
                    .frame_duration = 0.12f,
                    .loop = true,
                }
            );
        }
    }

    while (ecs_progress()) {
    }

    ecs_fini();
    return 0;
}
