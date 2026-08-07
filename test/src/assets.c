#include "test.h"

void assets_texture_path_uses_asset_root(void) {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_set_resource(SIAssetRoot, { .path = "../../../assets" });
    SITextureHandle texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);

    test_true(texture != SI_INVALID_HANDLE);
    siengine_release_texture(texture);

    ecs_fini();
}

void assets_animation_updates_sprite_frame(void) {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_entity_t entity = ecs_new();
    ecs_set(entity, SISprite, { .frame_index = 99 });
    ecs_set(entity, SIAnimation, {
        .start_index = 2,
        .end_index = 4,
        .frame_duration = 0.1f,
        .loop = true,
    });

    test_true(ecs_has(entity, SIAnimationTimer));
    test_int(2, ecs_get(entity, SISprite)->frame_index);
    test_true(ecs_get(entity, SIAnimationTimer)->playing);

    ecs_get(entity, SIAnimationTimer)->elapsed = 0.1f;
    ecs_run_phase(EcsOnUpdate);
    test_int(3, ecs_get(entity, SISprite)->frame_index);

    ecs_get(entity, SIAnimationTimer)->elapsed = 0.2f;
    ecs_run_phase(EcsOnUpdate);
    test_int(2, ecs_get(entity, SISprite)->frame_index);

    ecs_set(entity, SIAnimation, {
        .start_index = 5,
        .end_index = 6,
        .frame_duration = 0.1f,
        .loop = false,
    });
    test_int(5, ecs_get(entity, SISprite)->frame_index);

    ecs_get(entity, SIAnimationTimer)->elapsed = 0.2f;
    ecs_run_phase(EcsOnUpdate);
    test_int(6, ecs_get(entity, SISprite)->frame_index);
    test_false(ecs_get(entity, SIAnimationTimer)->playing);

    ecs_fini();
}
