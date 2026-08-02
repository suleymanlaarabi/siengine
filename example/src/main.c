#include "siecs.h"
#include "siengine.h"
#include "siui.h"
#include "siui_quartz.h"
#include <example.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
            .title = "siengine 2d",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .vsync = true,
        }
    );
    ecs_set_resource(SIUIRoot, { .render = app_render });

    ecs_entity_t camera = ecs_new();
    ecs_set(camera, Name, { "Camera" });
    ecs_set(camera, SICamera2D, { .zoom = 1.0f, .viewport_width = 320.0f, .viewport_height = 180.0f });
    ecs_add(camera, SIActiveCamera);

    for (int i = 0; i < 100; i++) {
        ecs_new();
    }

    while (ecs_progress()) {
    }

    ecs_fini();
    return 0;
}
