#include "siecs_rest.h"
#include "siengine.h"

int main(void) {
    ecs_with_features({ .target_fps = 60 });

    ECS_MODULE_IMPORT(siengine, {});
    ECS_MODULE_IMPORT(sirest, {});

    ecs_set_resource(SIWindow, { .title = "Hello" });
    ecs_set_resource(SIAssetRoot, { .path = "../../../../assets" });

    SITextureHandle texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);

    ecs_entity_t camera = ecs_new();
    ecs_set(
        camera,
        SICamera2D,
        { .zoom = 1.0f, .viewport_width = 320.0f, .viewport_height = 180.0f }
    );
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

    ecs_entity_t sprite = ecs_new();
    ecs_set(sprite, SISprite, { .texture = texture });
    ecs_set(
        sprite,
        SISpriteSheet,
        {
            .columns = 4,
            .rows = 1,
            .frame_width = 128,
            .frame_height = 128,
        }
    );

    ecs_run();
    return 0;
}
