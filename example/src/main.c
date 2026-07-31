#include "siecs.h"
#include "siengine.h"
#include "siui.h"
#include "siui_quartz.h"
#include <example.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void rotate_cube(ecs_iter_t *it) {
    SIRotation3d *rotations = ecs_field(it, 0);

    for (uint32_t i = 0; i < it->count; i++) {
        rotations[i].x += DEG2RAD(50) * it->delta_time;
    }
}

ecs_query_id_t entities_query = 0;

static ecs_entity_t entity_key(uint32_t index, uintptr_t query) {
    ecs_iter_t it = ecs_query_iter(query);

    while (ecs_iter_next(&it)) {
        if (index < it.count)
            return it.entities[index];

        index -= it.count;
    }

    return 0;
}

static sinode_desc_t
render_entity(sirender_ctx_t *_siui_render_ctx, uint32_t index, uint64_t key, void *data) {
    return node(height(px(44)), children(button(children(text(ecs_entity_name(key))))));
}

component(entity_list, {}) {
    return virtual_list(
            .count = ecs_query_count(entities_query),
            .item_extent = 44,
            .key = entity_key,
            .render_item = render_entity,
            .data = (void *)(uintptr_t)entities_query,
            .overscan = 2,
            .width = percent(100),
            .height = percent(100)
    );
}

component(app, {}) {
    return node(
        color(quartz_primary_text),
        width(px(250)),
        background(quartz_background),
        children(tabs_root(
                .default_selected = "entities",
                children(
                    tabs_list(children(
                        tabs_trigger("entities"),
                        tabs_trigger("systems"),
                        tabs_trigger("components")
                    )),
                    tabs_content("entities", children(ui(entity_list))),
                    tabs_content("systems", children(text("Systems"))),
                    tabs_content("components", children(text("Components")))
                )
        ))
    );
}

int main() {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    entities_query = ecs_query({});

    ecs_set_resource(
        SIWindow,
        {
            .title = "siengine cubes",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .vsync = true,
        }
    );
    ecs_set_resource(SIUIRoot, { .render = app_render });

    ecs_entity_t camera = ecs_new();
    ecs_set(camera, Name, { "Camera" });
    ecs_set(camera, SIPosition3d, { .x = 0.0f, .y = 1.5f, .z = -6.0f });
    ecs_set(camera, SIRotation3d, { .x = 0.0f, .y = 0.0f, .z = 0.0f });
    ecs_set(camera, SICamera3d, { .fov_y = 1.0471976f, .near_clip = 0.1f, .far_clip = 100.0f });
    ecs_add(camera, SIActiveCamera);

    ecs_entity_t cube = ecs_new();
    ecs_set(cube, Name, { "Cube" });
    ecs_add(cube, SICube);
    ecs_add(cube, Abstract);

    ecs_entity_t cube1 = ecs_new();
    ecs_is_a(cube1, cube);
    ecs_set(cube1, SIPosition3d, { -1.4f, 0.0f, 2.5f });
    ecs_set(cube1, SIRotation3d, { 0.2f, 0.3f, 0.0f });
    ecs_set(cube1, SIScale3d, { 1.0f, 1.0f, 1.0f });
    ecs_set(cube1, SIColor, { 0.8f, 0.2f, 0.15f, 1.0f });

    ecs_entity_t cube2 = ecs_new();
    ecs_is_a(cube2, cube);
    ecs_set(cube2, SIPosition3d, { 0.0f, 0.0f, 3.0f });
    ecs_set(cube2, SIRotation3d, { 0.0f, 0.5f, 0.2f });
    ecs_set(cube2, SIScale3d, { 0.8f, 1.4f, 0.8f });
    ecs_set(cube2, SIColor, { 0.2f, 0.65f, 0.95f, 1.0f });

    ecs_entity_t cube3 = ecs_new();
    ecs_is_a(cube3, cube);
    ecs_set(cube3, SIPosition3d, { 1.4f, 0.0f, 2.5f });
    ecs_set(cube3, SIRotation3d, { -0.3f, -0.4f, 0.1f });
    ecs_set(cube3, SIScale3d, { 1.0f, 0.7f, 1.2f });
    ecs_set(cube3, SIColor, { 0.25f, 0.9f, 0.35f, 1.0f });

    for (int i = 0; i < 100; i++) {
        ecs_new();
    }

    ecs_system(
        {
            .phase = EcsOnUpdate,
            .query.terms = { ecs_inout(SIRotation3d), ecs_filter(SICube) },
            .callback = rotate_cube,
        }
    );

    while (ecs_progress()) {
    }

    ecs_fini();
    return 0;
}
