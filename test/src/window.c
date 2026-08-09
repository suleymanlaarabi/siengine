#include "test.h"

void window_canvas_id_is_stored(void) {
    ecs_init();
    ECS_MODULE_IMPORT(siengine, {});

    ecs_set_resource(SIWindow, { .canvas_id = "siengine-canvas" });
    test_str("siengine-canvas", ecs_get_resource(SIWindow)->canvas_id);

    ecs_fini();
}
