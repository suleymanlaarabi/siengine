
/* A friendly warning from bake.test
 * ----------------------------------------------------------------------------
 * This file is generated. To add/remove testcases modify the 'project.json' of
 * the test project. ANY CHANGE TO THIS FILE IS LOST AFTER (RE)BUILDING!
 * ----------------------------------------------------------------------------
 */

#include <test.h>

// Testsuite 'component'
void component_cube_query_matches_transform_and_color(void);
void component_camera_query_matches_active_camera(void);
void component_camera_query_matches_multiple_active_cameras(void);
void component_cube_adds_render_components(void);
void component_position_does_not_add_rotation_or_scale(void);
void component_camera_adds_position_and_rotation(void);

// Testsuite 'math'
void math_model_applies_translation_scale(void);
void math_perspective_has_expected_terms(void);

// Testsuite 'ui'
void ui_root_can_be_set_after_window(void);
void ui_root_can_be_set_before_window(void);

bake_test_case component_testcases[] = {
    {
        "cube_query_matches_transform_and_color",
        component_cube_query_matches_transform_and_color
    },
    {
        "camera_query_matches_active_camera",
        component_camera_query_matches_active_camera
    },
    {
        "camera_query_matches_multiple_active_cameras",
        component_camera_query_matches_multiple_active_cameras
    },
    {
        "cube_adds_render_components",
        component_cube_adds_render_components
    },
    {
        "position_does_not_add_rotation_or_scale",
        component_position_does_not_add_rotation_or_scale
    },
    {
        "camera_adds_position_and_rotation",
        component_camera_adds_position_and_rotation
    }
};

bake_test_case math_testcases[] = {
    {
        "model_applies_translation_scale",
        math_model_applies_translation_scale
    },
    {
        "perspective_has_expected_terms",
        math_perspective_has_expected_terms
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
        "component",
        NULL,
        NULL,
        6,
        component_testcases
    },
    {
        "math",
        NULL,
        NULL,
        2,
        math_testcases
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
    return bake_test_run("siengine.test", argc, argv, suites, 3);
}
