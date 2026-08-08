#include "../../src/render_internal.h"
#include "test.h"
#include <stdlib.h>

static void import_engine(void) {
    ECS_MODULE_IMPORT(siengine, {});
}

static SITextureHandle make_test_texture(void) {
    SIAssetStore *assets = ecs_resource(SIAssetStore);
    assets->textures = calloc(2, sizeof(*assets->textures));
    assets->texture_count = 1;
    assets->texture_capacity = 2;
    assets->textures[1] = (SITextureSlot){
        .gpu = (SDL_GPUTexture *)(uintptr_t)1,
        .width = 64,
        .height = 64,
        .filter = SI_FILTER_NEAREST,
    };
    return 1;
}

void render_sprite_defaults_are_components(void) {
    ecs_init();
    import_engine();

    ecs_entity_t sprite = ecs_new();
    ecs_add(sprite, SISprite);

    test_true(ecs_has(sprite, SIColor));
    test_true(ecs_has(sprite, SISpriteFlip));
    test_true(ecs_has(sprite, SIPivot));
    test_true(ecs_has(sprite, SIBlendMode));
    test_true(ecs_get(sprite, SIColor)->r == 1.0f);
    test_true(ecs_get(sprite, SIColor)->g == 1.0f);
    test_true(ecs_get(sprite, SIColor)->b == 1.0f);
    test_true(ecs_get(sprite, SIColor)->a == 1.0f);
    test_true(ecs_get(sprite, SIPivot)->x == 0.5f);
    test_true(ecs_get(sprite, SIPivot)->y == 0.5f);
    test_int(SI_BLEND_NORMAL, ecs_get(sprite, SIBlendMode)->value);

    ecs_fini();
}

void render_extracts_once_for_multiple_views(void) {
    ecs_init();
    import_engine();
    SITextureHandle texture = make_test_texture();

    ecs_entity_t first_camera = ecs_new();
    ecs_add(first_camera, SICamera2D);

    ecs_entity_t second_camera = ecs_new();
    ecs_add(second_camera, SICamera2D);
    ecs_set(
        second_camera,
        SIWorldTransform2D,
        { .x = 1000000.0f, .scale_x = 1, .scale_y = 1 }
    );

    ecs_entity_t visible = ecs_new();
    ecs_set(visible, SISprite, { .texture = texture });

    ecs_entity_t hidden = ecs_new();
    ecs_set(hidden, SISprite, { .texture = texture });
    ecs_set(hidden, SIWorldTransform2D, { .x = 1000000.0f, .scale_x = 1, .scale_y = 1 });

    sirender_extract(NULL);

    SIRenderState *render = ecs_resource(SIRenderState);
    test_int(2, render->view_count);
    test_int(1, render->views[0].queue.count);
    test_int(1, render->views[1].queue.count);
    test_true(render->views[0].queue.commands[0].gpu_texture != NULL);
    test_true(render->views[0].queue.commands[0].gpu_texture ==
              render->views[1].queue.commands[0].gpu_texture);

    ecs_fini();
}

void render_extracts_sheet_region_and_layer_order(void) {
    ecs_init();
    import_engine();
    SITextureHandle texture = make_test_texture();

    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);

    ecs_entity_t background = ecs_new();
    ecs_set(background, SISprite, { .texture = texture });
    ecs_relate(background, Layer, SILayerBackground);

    ecs_entity_t foreground = ecs_new();
    ecs_set(foreground, SISprite, { .texture = texture, .frame_index = 5 });
    ecs_set(
        foreground,
        SISpriteSheet,
        {
            .columns = 4,
            .rows = 2,
            .frame_width = 32,
            .frame_height = 24,
            .margin_x = 2,
            .margin_y = 3,
            .spacing_x = 1,
            .spacing_y = 2,
        }
    );
    ecs_relate(foreground, Layer, SILayerForeground);

    ecs_run_phase(EcsOnUpdate);
    sirender_extract(NULL);

    SIRenderQueue *queue = &ecs_resource(SIRenderState)->views[0].queue;
    test_int(2, queue->count);
    test_true(queue->commands[0].layer == SILayerBackground);
    test_true(queue->commands[1].layer == SILayerForeground);
    test_true(queue->commands[1].width == 32.0f);
    test_true(queue->commands[1].height == 24.0f);
    test_true(queue->commands[1].u0 > 0.0f);
    test_true(queue->commands[1].u0 < queue->commands[1].u1);
    test_true(queue->commands[1].v0 > 0.0f);
    test_true(queue->commands[1].v0 < queue->commands[1].v1);

    ecs_fini();
}
