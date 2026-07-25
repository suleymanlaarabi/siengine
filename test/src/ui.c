#include "test.h"

component(test_ui, {}) {
    return node();
}

static void register_engine(void) {
    ECS_MODULE_IMPORT(siengine, {});
}

void ui_root_is_separate_from_window(void) {
    ecs_init();
    register_engine();

    ecs_entity_t window = ecs_new();
    ecs_set(window, SIWindow, {
        .title = "ui test",
        .width = 320,
        .height = 240,
    });
    ecs_set(window, SIUIRoot, { .render = test_ui_render });

    test_true(ecs_has(window, SIWindow));
    test_true(ecs_has(window, SIUIRoot));

    ecs_remove(window, SIUIRoot);
    test_false(ecs_has(window, SIUIRoot));

    ecs_fini();
}

void ui_root_can_be_added_before_window(void) {
    ecs_init();
    register_engine();

    ecs_entity_t window = ecs_new();
    ecs_set(window, SIUIRoot, { .render = test_ui_render });
    ecs_set(window, SIWindow, {
        .title = "ui test",
        .width = 320,
        .height = 240,
    });

    test_true(ecs_has(window, SIWindow));
    test_true(ecs_has(window, SIUIRoot));

    ecs_fini();
}
