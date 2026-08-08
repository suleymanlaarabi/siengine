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

    ecs_entity_t circle = ecs_new();
    ecs_set(circle, SICircle, { .radius = 24.0f });
    ecs_set(circle, SIColor, { .r = 0.95f, .g = 0.2f, .b = 0.2f, .a = 1.0f });
    ecs_set(circle, SITransform2D, { .x = -80.0f, .y = 0.0f });

    ecs_entity_t rectangle = ecs_new();
    ecs_set(rectangle, SIRectangle, { .width = 56.0f, .height = 28.0f });
    ecs_set(rectangle, SIColor, { .r = 0.2f, .g = 0.85f, .b = 0.3f, .a = 1.0f });
    ecs_set(rectangle, SITransform2D, { .x = 0.0f, .y = 0.0f });

    ecs_entity_t triangle = ecs_new();
    ecs_set(triangle, SITriangle, { .base = 48.0f, .height = 48.0f });
    ecs_set(triangle, SIColor, { .r = 0.2f, .g = 0.45f, .b = 1.0f, .a = 1.0f });
    ecs_set(triangle, SITransform2D, { .x = 80.0f, .y = 0.0f });

    ecs_run();
    return 0;
}
