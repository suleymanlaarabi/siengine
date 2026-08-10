#include "../../src/engine_internal.h"
#include "test.h"

ECS_MODULE_DECLARE(siscene2d, {});

static void register_scene2d(void) { ECS_MODULE_IMPORT(siscene2d, {}); }

void scene2d_transform_adds_world_transform(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t entity = ecs_new();
    ecs_add(entity, SITransform2D);
    ecs_set(
        entity,
        SITransform2D,
        {
            .x = 12.0f,
            .y = 24.0f,
            .rotation = 0.5f,
            .scale_x = 2.0f,
            .scale_y = 3.0f,
        }
    );

    test_true(ecs_has(entity, SIWorldTransform2D));

    SITransform2D *local = ecs_get(entity, SITransform2D);
    test_assert(local->x == 12.0f);
    test_assert(local->y == 24.0f);
    test_assert(local->rotation == 0.5f);
    test_assert(local->scale_x == 2.0f);
    test_assert(local->scale_y == 3.0f);

    ecs_fini();
}

void scene2d_world_transform_updates_multiple_entities(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t entities[10];
    for (uint32_t i = 0; i < 10; i++) {
        entities[i] = ecs_new();
        ecs_set(
            entities[i],
            SITransform2D,
            {
                .x = 10.0f + i,
                .y = 20.0f + i,
                .rotation = 0.1f * i,
                .scale_x = 2.0f + i,
                .scale_y = 3.0f + i,
            }
        );
    }

    ecs_run_phase(EcsPostUpdate);

    for (uint32_t i = 0; i < 10; i++) {
        SIWorldTransform2D *world = ecs_get(entities[i], SIWorldTransform2D);
        test_assert(world->x == 10.0f + i);
        test_assert(world->y == 20.0f + i);
        test_assert(world->rotation == 0.1f * i);
        test_assert(world->scale_x == 2.0f + i);
        test_assert(world->scale_y == 3.0f + i);
    }

    ecs_fini();
}

void scene2d_camera_requires_transform(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);
    ecs_set(
        camera,
        SICamera2D,
        {
            .zoom = 1.0f,
            .viewport_width = 320.0f,
            .viewport_height = 180.0f,
        }
    );

    test_true(ecs_has(camera, SITransform2D));
    test_true(ecs_has(camera, SIWorldTransform2D));
    test_true(ecs_has(camera, SICameraViewport));
    test_assert(ecs_get(camera, SICamera2D)->viewport_width == 320.0f);

    ecs_fini();
}

void scene2d_camera_add_initializes_defaults(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);

    SICamera2D *value = ecs_get(camera, SICamera2D);
    test_assert(value->zoom == 1.0f);
    test_assert(value->viewport_width == 320.0f);
    test_assert(value->viewport_height == 180.0f);

    ecs_fini();
}

void scene2d_query_matches_enabled_cameras(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t enabled = ecs_new();
    ecs_add(enabled, SICamera2D);

    ecs_entity_t disabled = ecs_new();
    ecs_add(disabled, SICamera2D);
    ecs_add(disabled, Disabled);

    ecs_query_id_t query = ecs_query(
        { .terms = {
              ecs_in(SICamera2D),
              ecs_in(SITransform2D),
          } }
    );
    ecs_iter_t it = ecs_query_iter(query);
    uint32_t count = 0;

    while (ecs_iter_next(&it)) {
        count += it.count;
    }

    test_int(1, count);

    ecs_query_fini(query);
    ecs_fini();
}

void scene2d_virtual_resolution_add_initializes_defaults(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SIVirtualResolution);

    SIVirtualResolution *resolution = ecs_get(camera, SIVirtualResolution);
    test_int(320, resolution->width);
    test_int(180, resolution->height);
    test_true(resolution->pixel_perfect);

    ecs_fini();
}

void scene2d_child_of_keeps_native_parent_relation(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t parent = ecs_new();
    ecs_entity_t child = ecs_new();
    ecs_relate(child, ChildOf, parent);

    test_assert(ecs_target(child, ChildOf) == parent);

    ecs_fini();
}

void scene2d_world_transform_follows_parent(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t parent = ecs_new();
    ecs_set(
        parent,
        SITransform2D,
        {
            .x = 10.0f,
            .y = 20.0f,
            .scale_x = 2.0f,
            .scale_y = 3.0f,
        }
    );

    ecs_entity_t child = ecs_new();
    ecs_set(child, SITransform2D, { .x = 4.0f, .y = 5.0f, .scale_x = 1.0f, .scale_y = 1.0f });
    ecs_relate(child, ChildOf, parent);

    ecs_run_phase(EcsPostUpdate);

    SIWorldTransform2D *world = ecs_get(child, SIWorldTransform2D);
    test_assert(world->x == 18.0f);
    test_assert(world->y == 35.0f);
    test_assert(world->scale_x == 2.0f);
    test_assert(world->scale_y == 3.0f);

    ecs_fini();
}

void scene2d_default_layers_are_ordered(void) {
    ecs_init();
    register_scene2d();

    test_true(SILayerBackground != 0);
    test_true(SILayerUI != 0);
    test_true(SILayerBackground < SILayerUI);

    ecs_entity_t background = ecs_new();
    ecs_add(background, SISprite);
    ecs_relate(background, Layer, SILayerBackground);

    ecs_entity_t ui = ecs_new();
    ecs_add(ui, SISprite);
    ecs_relate(ui, Layer, SILayerUI);

    ecs_query_id_t query = ecs_query(
        {
            .terms = { ecs_in(SISprite) },
            .relations = { ecs_rel(Layer) },
            .order_by = ecs_order_by_target(Layer),
        }
    );
    ecs_iter_t it = ecs_query_iter(query);
    test_true(ecs_iter_next(&it));
    test_assert(ecs_target_shared(&it, Layer) == SILayerBackground);
    test_assert(it.entities[0] == background);
    test_true(ecs_iter_next(&it));
    test_assert(ecs_target_shared(&it, Layer) == SILayerUI);
    test_assert(it.entities[0] == ui);
    test_false(ecs_iter_next(&it));

    ecs_query_fini(query);
    ecs_fini();
}

void scene2d_sprite_requires_transform(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t sprite = ecs_new();
    ecs_add(sprite, SISprite);

    test_true(ecs_has(sprite, SITransform2D));
    test_true(ecs_has(sprite, SIWorldTransform2D));

    ecs_fini();
}

void scene2d_shapes_require_transform_and_default_layer(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t circle = ecs_new();
    ecs_add(circle, SICircle);
    ecs_entity_t rectangle = ecs_new();
    ecs_add(rectangle, SIRectangle);
    ecs_entity_t triangle = ecs_new();
    ecs_add(triangle, SITriangle);

    test_true(ecs_has(circle, SITransform2D));
    test_true(ecs_has(circle, SIWorldTransform2D));
    test_true(ecs_has(circle, SIColor));
    test_assert(ecs_target(circle, Layer) == SILayerWorld);
    test_assert(ecs_target(rectangle, Layer) == SILayerWorld);
    test_assert(ecs_target(triangle, Layer) == SILayerWorld);
    test_assert(ecs_get(circle, SICircle)->radius == 1.0f);
    test_assert(ecs_get(rectangle, SIRectangle)->width == 1.0f);
    test_assert(ecs_get(rectangle, SIRectangle)->height == 1.0f);
    test_assert(ecs_get(triangle, SITriangle)->base == 1.0f);
    test_assert(ecs_get(triangle, SITriangle)->height == 1.0f);

    ecs_fini();
}

void scene2d_sprite_gets_default_world_layer(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t sprite = ecs_new();
    ecs_add(sprite, SISprite);

    test_true(ecs_has(sprite, SISprite));
    test_true(ecs_has(sprite, SIRenderable));
    test_true(ecs_has(sprite, SITransform2D));
    test_true(ecs_has(sprite, SIWorldTransform2D));
    test_true(ecs_has(sprite, SIColor));
    test_true(ecs_has(sprite, SISpriteFlip));
    test_true(ecs_target(sprite, Layer) == SILayerWorld);

    ecs_fini();
}

void scene2d_explicit_layer_overrides_default(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t sprite = ecs_new();
    ecs_add(sprite, SISprite);
    test_true(ecs_target(sprite, Layer) == SILayerWorld);

    ecs_relate(sprite, Layer, SILayerUI);
    test_true(ecs_target(sprite, Layer) == SILayerUI);

    ecs_fini();
}

void scene2d_sprite_sheet_describes_grid(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t sprite = ecs_new();
    ecs_set(sprite, SISprite, { .frame_index = 5 });
    ecs_set(
        sprite,
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

    SISprite *current = ecs_get(sprite, SISprite);
    SISpriteSheet *sheet = ecs_get(sprite, SISpriteSheet);
    test_assert(current->frame_index == 5);
    test_assert(sheet->columns == 4);
    test_assert(sheet->rows == 2);
    test_assert(sheet->frame_width == 32);
    test_assert(sheet->frame_height == 24);
    test_assert(sheet->margin_x == 2);
    test_assert(sheet->margin_y == 3);
    test_assert(sheet->spacing_x == 1);
    test_assert(sheet->spacing_y == 2);

    ecs_fini();
}
