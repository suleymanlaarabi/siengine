#include "test.h"
#include "../../src/assets_internal.h"

ECS_COMPONENT_DECLARE(AssetsTextureTrigger, { int value; });
ECS_COMPONENT_DEFINE(AssetsTextureTrigger);

static SITextureHandle worker_texture;

static void load_texture_from_worker(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    worker_texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);
}

void assets_texture_path_uses_asset_root(void) {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_set_resource(SIAssetRoot, { .path = "../../../assets" });
    SITextureHandle texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);

    test_true(texture != SI_INVALID_HANDLE);
    siengine_release_texture(texture);

    ecs_fini();
}

void assets_live_texture_is_released_during_ecs_fini(void) {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_set_resource(SIAssetRoot, { .path = "../../../assets" });
    SITextureHandle texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);
    test_true(texture != SI_INVALID_HANDLE);

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

void assets_texture_load_from_worker_is_queued(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    ECS_MODULE_IMPORT(siengine, {});
    ecs_set_resource(SIAssetRoot, { .path = "../../../assets" });
    ECS_COMPONENT_REGISTER(AssetsTextureTrigger);

    ecs_entity_t trigger = ecs_new();
    ecs_add(trigger, AssetsTextureTrigger);
    ecs_system({
        .name = "LoadTextureFromWorker",
        .phase = EcsOnUpdate,
        .callback = load_texture_from_worker,
        .query = { .terms = { ecs_inout(AssetsTextureTrigger) } },
    });

    ecs_run_phase(EcsOnUpdate);
    test_true(worker_texture != SI_INVALID_HANDLE);
    test_int(SI_TEXTURE_PENDING, ecs_get(worker_texture, SITexture)->state);

    ecs_run_phase(EcsPreUpdate);
    test_int(SI_TEXTURE_READY, ecs_get(worker_texture, SITexture)->state);

    siengine_release_texture(worker_texture);
    ecs_run_phase(ecs_get_resource(SIAssetState)->release_phase);
    test_false(ecs_is_alive(worker_texture));
    ecs_fini();
}
