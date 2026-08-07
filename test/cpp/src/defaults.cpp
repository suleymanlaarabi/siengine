#include <cstdint>
#include <bake_test.h>
#include <siecs.h>
#include <siengine.h>

ECS_MODULE_DECLARE(siscene2d, {});

static void register_scene2d() {
    ecs::import<siscene2d>();
}

void defaults_component_defaults(void) {
    SIWindow window{ "Hello" };

    test_str("Hello", window.title);
    test_true(SIWindow{}.width == 1280);
    test_true(SIWindow{}.height == 720);
    test_true(SIWindow{}.resizable);
    test_true(SIWindow{}.vsync);
    test_str("Siengine", SIWindow{}.title);
    test_str("./assets", SIAssetRoot{}.path);

    test_true(SICamera2D{}.zoom == 1.0f);
    test_true(SICamera2D{}.viewport_width == 320.0f);
    test_true(SICamera2D{}.viewport_height == 180.0f);
    test_true(SITransform2D{}.scale_x == 1.0f);
    test_true(SITransform2D{}.scale_y == 1.0f);
    test_true(SIColor{}.r == 1.0f);
    test_true(SIColor{}.g == 1.0f);
    test_true(SIColor{}.b == 1.0f);
    test_true(SIColor{}.a == 1.0f);
    test_true(SIPivot{}.x == 0.5f);
    test_true(SIPivot{}.y == 0.5f);
    test_true(SISpriteFlip{}.x == false);
    test_true(SISpriteFlip{}.y == false);
    test_true(SIBlendMode{}.value == SI_BLEND_NORMAL);
    test_true(SISprite{}.frame_index == 0);
}

void defaults_camera_defaults(void) {
    ecs_init();
    register_scene2d();

    auto camera = ecs::entity::create().set(SICamera2D{});

    test_true(camera.has<SICamera2D>());
    test_true(camera.has<SITransform2D>());
    test_true(camera.has<SIWorldTransform2D>());
    test_true(camera.has<SICameraViewport>());
    test_true(camera.has<SIActiveCamera>());
    test_true(camera.get<SITransform2D>().scale_x == 1.0f);
    test_true(camera.get<SITransform2D>().scale_y == 1.0f);
    test_true(camera.get<SICameraViewport>().width == 1.0f);
    test_true(camera.get<SICameraViewport>().height == 1.0f);

    camera.remove<SIActiveCamera>();
    test_false(camera.has<SIActiveCamera>());

    ecs_fini();
}

void defaults_sprite_default_layer(void) {
    ecs_init();
    register_scene2d();

    auto sprite = ecs::entity::create().set(SISprite{});

    test_true(sprite.has<SITransform2D>());
    test_true(sprite.has<SIWorldTransform2D>());
    test_int(SILayerActors, sprite.target<Layer>().id());

    ecs_fini();
}

void defaults_sprite_explicit_layer(void) {
    ecs_init();
    register_scene2d();

    auto sprite = ecs::entity::create().relate<Layer>(SILayerEffects).set(SISprite{});

    test_int(SILayerEffects, sprite.target<Layer>().id());

    ecs_fini();
}

void defaults_asset_root_default(void) {
    ecs_init();
    ecs::import<siengine>();

    test_str("./assets", ecs_get_resource(SIAssetRoot)->path);

    ecs_fini();
}
