#include "../../src/render_internal.h"
#include "test.h"

static void import_engine(void) { ECS_MODULE_IMPORT(siengine, {}); }

static SITextureHandle make_test_texture(void) {
    return siengine_load_texture("hero.png", SI_FILTER_NEAREST);
}

static ecs_entity_t make_material(SITextureHandle texture) {
    ecs_entity_t material = ecs_new();
    ecs_set(
        material,
        SIMaterial2D,
        {
            .texture = texture,
            .filter = SI_FILTER_NEAREST,
        }
    );
    return material;
}

void render_queries_are_world_owned_across_cycles(void) {
    for (uint32_t cycle = 0; cycle < 2; cycle++) {
        ecs_init();
        import_engine();

        SIRenderState *render = ecs_resource(SIRenderState);
        test_true(render->camera_query != 0);
        test_true(render->renderable_query != 0);

        ecs_fini();
    }
}

void render_sprite_defaults_are_components(void) {
    ecs_init();
    import_engine();

    ecs_entity_t sprite = ecs_new();
    ecs_add(sprite, SISprite);
    ecs_relate(sprite, Material, SI2DDefaultMaterial);

    test_true(ecs_has(sprite, SIColor));
    test_true(ecs_has(sprite, SISpriteFlip));
    test_true(ecs_has_relation(sprite, Material));
    test_true(ecs_get(sprite, SIColor)->r == 1.0f);
    test_true(ecs_get(sprite, SIColor)->g == 1.0f);
    test_true(ecs_get(sprite, SIColor)->b == 1.0f);
    test_true(ecs_get(sprite, SIColor)->a == 1.0f);

    ecs_fini();
}

void render_extracts_once_for_multiple_views(void) {
    ecs_init();
    import_engine();
    SITextureHandle texture = make_test_texture();
    ecs_entity_t material = make_material(texture);

    ecs_entity_t first_camera = ecs_new();
    ecs_add(first_camera, SICamera2D);
    ecs_entity_t second_camera = ecs_new();
    ecs_add(second_camera, SICamera2D);

    ecs_entity_t visible = ecs_new();
    ecs_add(visible, SISprite);
    ecs_relate(visible, Material, material);

    sirender_extract(NULL);

    SIRenderState *render = ecs_resource(SIRenderState);
    test_int(2, render->views.size);
    test_int(1, render->batches.size);
    test_int(1, render->instances.size);
    test_int(1, sicore_vec_get(&render->batches, 0, SIRenderBatch)->instance_count);
    test_int(texture, sicore_vec_get(&render->batches, 0, SIRenderBatch)->texture);
    test_true(sicore_vec_get(&render->batches, 0, SIRenderBatch)->pipeline == SI_PIPELINE_SPRITE);
    test_true(sicore_vec_get(&render->batches, 0, SIRenderBatch)->geometry == SI_GEOMETRY_QUAD);

    ecs_fini();
}

void render_extracts_sheet_region_and_layer_order(void) {
    ecs_init();
    import_engine();
    SITextureHandle texture = make_test_texture();
    ecs_entity_t material = make_material(texture);
    ecs_set(
        material,
        SIMaterial2D,
        { .texture = texture, .filter = SI_FILTER_NEAREST }
    );
    ecs_set(material, SISpriteSheet, {
        .columns = 4,
        .rows = 2,
        .frame_width = 32,
        .frame_height = 24,
        .margin_x = 2,
        .margin_y = 3,
        .spacing_x = 1,
        .spacing_y = 2,
    });

    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);
    ecs_entity_t background = ecs_new();
    ecs_add(background, SISprite);
    ecs_relate(background, Layer, SILayerBackground);
    ecs_relate(background, Material, material);
    ecs_entity_t foreground = ecs_new();
    ecs_set(foreground, SISprite, { .frame_index = 5 });
    ecs_relate(foreground, Layer, SILayerForeground);
    ecs_relate(foreground, Material, material);

    sirender_extract(NULL);
    SIRenderState *render = ecs_resource(SIRenderState);
    test_int(2, render->batches.size);
    test_true(sicore_vec_get(&render->batches, 0, SIRenderBatch)->layer == SILayerBackground);
    test_true(sicore_vec_get(&render->batches, 1, SIRenderBatch)->layer == SILayerForeground);
    test_true(sicore_vec_get(&render->batches, 1, SIRenderBatch)->has_sheet);
    test_int(32, sicore_vec_get(&render->instances, 1, SIInstance2D)->width);
    test_int(24, sicore_vec_get(&render->instances, 1, SIInstance2D)->height);
    test_int(5, sicore_vec_get(&render->instances, 1, SIInstance2D)->frame_index);

    ecs_fini();
}

void render_extracts_colored_shapes(void) {
    ecs_init();
    import_engine();
    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);
    ecs_entity_t circle = ecs_new();
    ecs_set(circle, SICircle, { .radius = 12.0f });
    ecs_relate(circle, Material, SI2DDefaultMaterial);
    ecs_set(circle, SIColor, { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f });
    ecs_entity_t rectangle = ecs_new();
    ecs_set(rectangle, SIRectangle, { .width = 20.0f, .height = 8.0f });
    ecs_relate(rectangle, Material, SI2DDefaultMaterial);
    ecs_set(rectangle, SIColor, { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f });
    ecs_entity_t triangle = ecs_new();
    ecs_set(triangle, SITriangle, { .base = 18.0f, .height = 24.0f });
    ecs_relate(triangle, Material, SI2DDefaultMaterial);
    ecs_set(triangle, SIColor, { .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f });

    sirender_extract(NULL);
    SIRenderState *render = ecs_resource(SIRenderState);
    test_int(3, render->batches.size);
    test_int(3, render->instances.size);
    test_true(sicore_vec_get(&render->batches, 0, SIRenderBatch)->pipeline == SI_PIPELINE_CIRCLE);
    test_true(sicore_vec_get(&render->batches, 0, SIRenderBatch)->geometry == SI_GEOMETRY_QUAD);
    test_true(sicore_vec_get(&render->batches, 1, SIRenderBatch)->pipeline == SI_PIPELINE_SHAPE);
    test_true(sicore_vec_get(&render->batches, 1, SIRenderBatch)->geometry == SI_GEOMETRY_QUAD);
    test_true(sicore_vec_get(&render->batches, 2, SIRenderBatch)->pipeline == SI_PIPELINE_SHAPE);
    test_true(sicore_vec_get(&render->batches, 2, SIRenderBatch)->geometry == SI_GEOMETRY_TRIANGLE);
    test_int(0, sicore_vec_get(&render->batches, 0, SIRenderBatch)->instance_offset);
    test_int(1, sicore_vec_get(&render->batches, 1, SIRenderBatch)->instance_offset);
    test_int(2, sicore_vec_get(&render->batches, 2, SIRenderBatch)->instance_offset);
    test_true(sicore_vec_get(&render->instances, 0, SIInstance2D)->color.r == 1.0f);
    test_true(sicore_vec_get(&render->instances, 1, SIInstance2D)->color.g == 1.0f);
    test_true(sicore_vec_get(&render->instances, 2, SIInstance2D)->color.b == 1.0f);

    ecs_fini();
}

void render_culls_shapes(void) {
    ecs_init();
    import_engine();
    ecs_entity_t camera = ecs_new();
    ecs_add(camera, SICamera2D);
    ecs_entity_t visible = ecs_new();
    ecs_set(visible, SICircle, { .radius = 4.0f });
    ecs_relate(visible, Material, SI2DDefaultMaterial);
    ecs_entity_t hidden = ecs_new();
    ecs_set(hidden, SIRectangle, { .width = 8.0f, .height = 8.0f });
    ecs_relate(hidden, Material, SI2DDefaultMaterial);
    ecs_set(hidden, Position, { .x = 1000000.0f, .y = 0.0f });

    ecs_run_phase(EcsPostUpdate);
    sirender_extract(NULL);
    test_int(1, ecs_resource(SIRenderState)->instances.size);
    test_int(1, ecs_resource(SIRenderState)->batches.size);

    ecs_fini();
}
