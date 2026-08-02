#include "test.h"

ECS_MODULE_DECLARE(siscene2d, {});

static void register_scene2d(void) {
    ECS_MODULE_IMPORT(siscene2d, {});
}

void scene2d_transform_adds_world_transform(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t entity = ecs_new();
    ecs_add(entity, SITransform2D);
    ecs_set(entity, SITransform2D, {
        .x = 12.0f,
        .y = 24.0f,
        .rotation = 0.5f,
        .scale_x = 2.0f,
        .scale_y = 3.0f,
    });

    test_true(ecs_has(entity, SIWorldTransform2D));

    SITransform2D *local = ecs_get(entity, SITransform2D);
    test_assert(local->x == 12.0f);
    test_assert(local->y == 24.0f);
    test_assert(local->rotation == 0.5f);
    test_assert(local->scale_x == 2.0f);
    test_assert(local->scale_y == 3.0f);

    ecs_fini();
}

void scene2d_camera_requires_transform(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);
    ecs_set(camera, SICamera2D, {
        .zoom = 1.0f,
        .viewport_width = 320.0f,
        .viewport_height = 180.0f,
    });
    ecs_add(camera, SIActiveCamera);

    test_true(ecs_has(camera, SITransform2D));
    test_true(ecs_has(camera, SIWorldTransform2D));
    test_true(ecs_has(camera, SIActiveCamera));
    test_assert(ecs_get(camera, SICamera2D)->viewport_width == 320.0f);

    ecs_fini();
}

void scene2d_query_matches_active_cameras(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t first = ecs_new();
    ecs_add(first, SICamera2D);
    ecs_add(first, SIActiveCamera);

    ecs_entity_t second = ecs_new();
    ecs_add(second, SICamera2D);
    ecs_add(second, SIActiveCamera);

    ecs_entity_t inactive = ecs_new();
    ecs_add(inactive, SICamera2D);

    ecs_query_id_t query = ecs_query(
        { .terms = {
              ecs_in(SICamera2D),
              ecs_in(SITransform2D),
              ecs_filter(SIActiveCamera),
          } }
    );
    ecs_iter_t it = ecs_query_iter(query);
    uint32_t count = 0;

    while (ecs_iter_next(&it)) {
        count += it.count;
    }

    test_int(2, count);

    ecs_query_fini(query);
    ecs_fini();
}

void scene2d_child_of_keeps_native_parent_relation(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t parent = ecs_new();
    ecs_entity_t child = ecs_new();
    ecs_set(child, ChildOf, { parent });

    ChildOf *relation = ecs_get(child, ChildOf);
    test_true(relation != NULL);
    test_assert(relation->target == parent);

    ecs_query_id_t query = ecs_query({ .terms = { ecs_in(ChildOf) } });
    ecs_iter_t it = ecs_query_iter(query);
    test_true(ecs_iter_next(&it));
    test_int(1, it.count);
    test_assert(it.entities[0] == child);
    test_false(ecs_iter_next(&it));

    ecs_query_fini(query);
    ecs_fini();
}

void scene2d_render_order_is_component_data(void) {
    ecs_init();
    register_scene2d();

    ecs_entity_t entity = ecs_new();
    ecs_set(entity, SIRenderOrder, { .layer = 4, .order = -12 });
    ecs_set(entity, SIColor, { .r = 0.2f, .g = 0.4f, .b = 0.6f, .a = 1.0f });

    SIRenderOrder *order = ecs_get(entity, SIRenderOrder);
    SIColor *color = ecs_get(entity, SIColor);
    test_int(4, order->layer);
    test_int(-12, order->order);
    test_assert(color->b == 0.6f);

    ecs_fini();
}
