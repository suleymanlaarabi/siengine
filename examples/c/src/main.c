#include "siengine.h"

int main(void) {
    ecs_with_features({ .target_fps = 60 });

    ECS_MODULE_IMPORT(siengine, {});
    ecs_set_resource(SIWindow, { .title = "Hello", .canvas_id = "siengine-canvas" });
#if defined(__EMSCRIPTEN__)
    ecs_set_resource(SIAssetRoot, { .path = "/assets" });
#else
    ecs_set_resource(SIAssetRoot, { .path = "../../../../assets" });
#endif

    SITextureHandle texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);
    ecs_entity_t material = ecs_new();
    ecs_set(
        material,
        SIMaterial2D,
        {
            .texture = texture,
            .filter = SI_FILTER_NEAREST,
        }
    );
    ecs_set(material, SISpriteSheet, {
        .columns = 4,
        .rows = 1,
        .frame_width = 128,
        .frame_height = 128,
    });

    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);
    ecs_add(camera, SIVirtualResolution);

    ecs_entity_t sprite = ecs_new();
    ecs_set(sprite, SISprite, {});
    ecs_relate(sprite, Layer, SILayerActors);
    ecs_relate(sprite, Material, material);
    ecs_set(sprite, Position, { .x = -40.0f, .y = 72.0f });

    ecs_entity_t second_sprite = ecs_new();
    ecs_set(second_sprite, SISprite, { .frame_index = 1 });
    ecs_relate(second_sprite, Layer, SILayerActors);
    ecs_relate(second_sprite, Material, material);
    ecs_set(second_sprite, Position, { .x = 40.0f, .y = 72.0f });

    ecs_entity_t circle = ecs_new();
    ecs_set(circle, SICircle, { .radius = 24.0f });
    ecs_relate(circle, Layer, SILayerActors);
    ecs_relate(circle, Material, SI2DDefaultMaterial);
    ecs_set(circle, SIColor, { .r = 0.95f, .g = 0.2f, .b = 0.2f, .a = 1.0f });
    ecs_set(circle, Position, { .x = -80.0f, .y = 0.0f });

    ecs_entity_t rectangle = ecs_new();
    ecs_set(rectangle, SIRectangle, { .width = 56.0f, .height = 28.0f });
    ecs_relate(rectangle, Layer, SILayerActors);
    ecs_relate(rectangle, Material, SI2DDefaultMaterial);
    ecs_set(rectangle, SIColor, { .r = 0.2f, .g = 0.85f, .b = 0.3f, .a = 1.0f });
    ecs_set(rectangle, Position, { .x = 0.0f, .y = 0.0f });

    ecs_entity_t triangle = ecs_new();
    ecs_set(triangle, SITriangle, { .base = 48.0f, .height = 48.0f });
    ecs_relate(triangle, Layer, SILayerActors);
    ecs_relate(triangle, Material, SI2DDefaultMaterial);
    ecs_set(triangle, SIColor, { .r = 0.2f, .g = 0.45f, .b = 1.0f, .a = 1.0f });
    ecs_set(triangle, Position, { .x = 80.0f, .y = 0.0f });

    siengine_run();
    return 0;
}
