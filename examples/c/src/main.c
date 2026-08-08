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
    ecs_add(camera, SICamera2D);
    ecs_add(camera, SIVirtualResolution);

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
