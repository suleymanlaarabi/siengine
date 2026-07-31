#include "test.h"

component(test_ui, {}) {
    return node();
}

static void register_engine(void) {
    ECS_MODULE_IMPORT(siengine, {});
}

void ui_root_can_be_set_after_window(void) {
    ecs_init();
    register_engine();

    ecs_set_resource(SIWindow, {
        .title = "ui test",
        .width = 320,
        .height = 240,
    });
    ecs_set_resource(SIUIRoot, { .render = test_ui_render });

    test_true(ecs_get_resource(SIWindow) != NULL);
    test_true(ecs_get_resource(SIUIRoot)->render == test_ui_render);

    ecs_fini();
}

void ui_root_can_be_set_before_window(void) {
    ecs_init();
    register_engine();

    ecs_set_resource(SIUIRoot, { .render = test_ui_render });
    ecs_set_resource(SIWindow, {
        .title = "ui test",
        .width = 320,
        .height = 240,
    });

    test_true(ecs_get_resource(SIWindow) != NULL);
    test_true(ecs_get_resource(SIUIRoot)->render == test_ui_render);

    ecs_fini();
}
