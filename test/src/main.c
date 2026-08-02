
/* A friendly warning from bake.test
 * ----------------------------------------------------------------------------
 * This file is generated. To add/remove testcases modify the 'project.json' of
 * the test project. ANY CHANGE TO THIS FILE IS LOST AFTER (RE)BUILDING!
 * ----------------------------------------------------------------------------
 */

#include <test.h>

// Testsuite 'scene2d'
void scene2d_transform_adds_world_transform(void);
void scene2d_camera_requires_transform(void);
void scene2d_query_matches_active_cameras(void);
void scene2d_child_of_keeps_native_parent_relation(void);
void scene2d_render_order_is_component_data(void);

// Testsuite 'ui'
void ui_root_can_be_set_after_window(void);
void ui_root_can_be_set_before_window(void);

bake_test_case scene2d_testcases[] = {
    {
        "transform_adds_world_transform",
        scene2d_transform_adds_world_transform
    },
    {
        "camera_requires_transform",
        scene2d_camera_requires_transform
    },
    {
        "query_matches_active_cameras",
        scene2d_query_matches_active_cameras
    },
    {
        "child_of_keeps_native_parent_relation",
        scene2d_child_of_keeps_native_parent_relation
    },
    {
        "render_order_is_component_data",
        scene2d_render_order_is_component_data
    }
};

bake_test_case ui_testcases[] = {
    {
        "root_can_be_set_after_window",
        ui_root_can_be_set_after_window
    },
    {
        "root_can_be_set_before_window",
        ui_root_can_be_set_before_window
    }
};


static bake_test_suite suites[] = {
    {
        "scene2d",
        NULL,
        NULL,
        5,
        scene2d_testcases
    },
    {
        "ui",
        NULL,
        NULL,
        2,
        ui_testcases
    }
};

int main(int argc, char *argv[]) {
    return bake_test_run("siengine.test", argc, argv, suites, 2);
}
