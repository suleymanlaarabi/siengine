#include "../../src/engine_internal.h"
#include "test.h"

ECS_MODULE_DECLARE(siscene2d, {});

static void register_scene2d(void) { ECS_MODULE_IMPORT(siscene2d, {}); }

void scene2d_world_transform_adds_spatial_components(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t entity = ecs_new();
    ecs_add(entity, SIWorldTransform2D);

    test_true(ecs_has(entity, Position));
    test_true(ecs_has(entity, Rotation));
    test_true(ecs_has(entity, SIScale2D));
    test_true(ecs_has(entity, SIWorldTransform2D));

    test_assert(ecs_get(entity, Position)->x == 0.0f);
    test_assert(ecs_get(entity, Position)->y == 0.0f);
    test_assert(ecs_get(entity, Rotation)->angle == 0.0f);
    test_assert(ecs_get(entity, SIScale2D)->x == 1.0f);
    test_assert(ecs_get(entity, SIScale2D)->y == 1.0f);

    ecs_set(entity, Position, { .x = 12.0f, .y = 24.0f });
    ecs_set(entity, Rotation, { .angle = 0.5f });
    ecs_set(entity, SIScale2D, { .x = 2.0f, .y = 3.0f });

    ecs_run_phase(EcsPostUpdate);

    SIWorldTransform2D *world = ecs_get(entity, SIWorldTransform2D);
    test_assert(world->x == 12.0f);
    test_assert(world->y == -24.0f);
    test_assert(world->rotation == 0.5f);
    test_assert(world->scale_x == 2.0f);
    test_assert(world->scale_y == 3.0f);

    ecs_fini();
}

void scene2d_imports_siphysics_automatically(void) {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_entity_t sprite = ecs_new();
    ecs_add(sprite, SISprite);

    test_true(ecs_has(sprite, Position));
    test_true(ecs_has(sprite, Rotation));
    test_true(ecs_has(sprite, SIScale2D));
    test_true(ecs_has(sprite, SIWorldTransform2D));

    ecs_fini();
}

void scene2d_world_transform_updates_multiple_entities(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t entities[10];
    for (uint32_t i = 0; i < 10; i++) {
        entities[i] = ecs_new();
        ecs_add(entities[i], SIWorldTransform2D);
        ecs_set(entities[i], Position, { .x = 10.0f + i, .y = 20.0f + i });
        ecs_set(entities[i], Rotation, { .angle = 0.1f * i });
        ecs_set(entities[i], SIScale2D, { .x = 2.0f + i, .y = 3.0f + i });
    }

    ecs_run_phase(EcsPostUpdate);

    for (uint32_t i = 0; i < 10; i++) {
        SIWorldTransform2D *world = ecs_get(entities[i], SIWorldTransform2D);
        test_assert(world->x == 10.0f + i);
        test_assert(world->y == -(20.0f + i));
        test_assert(world->rotation == 0.1f * i);
        test_assert(world->scale_x == 2.0f + i);
        test_assert(world->scale_y == 3.0f + i);
    }

    ecs_fini();
}

void scene2d_camera_requires_spatial_components(void) {
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

    test_true(ecs_has(camera, Position));
    test_true(ecs_has(camera, Rotation));
    test_true(ecs_has(camera, SIScale2D));
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
        {
            .components = {
                ecs_in(SICamera2D),
                ecs_in(SIWorldTransform2D),
            },
        }
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
    ecs_add(parent, SIWorldTransform2D);
    ecs_set(parent, Position, { .x = 10.0f, .y = 20.0f });
    ecs_set(parent, Rotation, { .angle = 0.0f });
    ecs_set(parent, SIScale2D, { .x = 2.0f, .y = 3.0f });

    ecs_entity_t child = ecs_new();
    ecs_add(child, SIWorldTransform2D);
    ecs_set(child, Position, { .x = 4.0f, .y = 5.0f });
    ecs_set(child, Rotation, { .angle = 0.5f });
    ecs_set(child, SIScale2D, { .x = 1.0f, .y = 1.0f });
    ecs_relate(child, ChildOf, parent);

    ecs_run_phase(EcsPostUpdate);

    SIWorldTransform2D *parent_world = ecs_get(parent, SIWorldTransform2D);
    test_assert(parent_world->x == 10.0f);
    test_assert(parent_world->y == -20.0f);
    test_assert(parent_world->rotation == 0.0f);
    test_assert(parent_world->scale_x == 2.0f);
    test_assert(parent_world->scale_y == 3.0f);

    SIWorldTransform2D *world = ecs_get(child, SIWorldTransform2D);
    test_assert(world->x == 18.0f);
    test_assert(world->y == -35.0f);
    test_assert(world->scale_x == 2.0f);
    test_assert(world->scale_y == 3.0f);
    test_assert(world->rotation == 0.5f);

    ecs_fini();
}

void scene2d_world_transform_updates_hierarchy_by_depth(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t root = ecs_new();
    ecs_add(root, SIWorldTransform2D);
    ecs_set(root, Position, { .x = 10.0f, .y = 20.0f });

    ecs_entity_t child = ecs_new();
    ecs_add(child, SIWorldTransform2D);
    ecs_set(child, Position, { .x = 4.0f, .y = 5.0f });
    ecs_relate(child, ChildOf, root);

    ecs_entity_t grandchild = ecs_new();
    ecs_add(grandchild, SIWorldTransform2D);
    ecs_set(grandchild, Position, { .x = 2.0f, .y = 3.0f });
    ecs_relate(grandchild, ChildOf, child);

    ecs_run_phase(EcsPostUpdate);

    const SIWorldTransform2D *root_world = ecs_get(root, SIWorldTransform2D);
    const SIWorldTransform2D *child_world = ecs_get(child, SIWorldTransform2D);
    const SIWorldTransform2D *grandchild_world = ecs_get(grandchild, SIWorldTransform2D);

    test_assert(root_world->x == 10.0f);
    test_assert(root_world->y == -20.0f);

    test_assert(child_world->x == 14.0f);
    test_assert(child_world->y == -25.0f);

    test_assert(grandchild_world->x == 16.0f);
    test_assert(grandchild_world->y == -28.0f);

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
            .components = {
                ecs_in(SISprite),
            },
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

void scene2d_sprite_requires_spatial_components(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t sprite = ecs_new();
    ecs_add(sprite, SISprite);

    test_true(ecs_has(sprite, Position));
    test_true(ecs_has(sprite, Rotation));
    test_true(ecs_has(sprite, SIScale2D));
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

    test_true(ecs_has(circle, Position));
    test_true(ecs_has(circle, Rotation));
    test_true(ecs_has(circle, SIScale2D));
    test_true(ecs_has(circle, SIWorldTransform2D));
    test_true(ecs_has(circle, SIColor));
    test_true(ecs_has(rectangle, Position));
    test_true(ecs_has(rectangle, Rotation));
    test_true(ecs_has(rectangle, SIScale2D));
    test_true(ecs_has(rectangle, SIWorldTransform2D));
    test_true(ecs_has(rectangle, SIColor));
    test_true(ecs_has(triangle, Position));
    test_true(ecs_has(triangle, Rotation));
    test_true(ecs_has(triangle, SIScale2D));
    test_true(ecs_has(triangle, SIWorldTransform2D));
    test_true(ecs_has(triangle, SIColor));
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
    test_true(ecs_has(sprite, Position));
    test_true(ecs_has(sprite, Rotation));
    test_true(ecs_has(sprite, SIScale2D));
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
