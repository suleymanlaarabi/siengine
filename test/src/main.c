
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

// Testsuite 'math'
void math_model_applies_translation_scale(void);
void math_perspective_has_expected_terms(void);

// Testsuite 'window'
void window_resize_inactive_without_event(void);
void window_resize_active_before_settle_delay(void);
void window_resize_inactive_at_settle_delay(void);
void window_resize_new_event_extends_nonblocking_period(void);

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

bake_test_case window_testcases[] = {
    {
        "resize_inactive_without_event",
        window_resize_inactive_without_event
    },
    {
        "resize_active_before_settle_delay",
        window_resize_active_before_settle_delay
    },
    {
        "resize_inactive_at_settle_delay",
        window_resize_inactive_at_settle_delay
    },
    {
        "resize_new_event_extends_nonblocking_period",
        window_resize_new_event_extends_nonblocking_period
    }
};


static bake_test_suite suites[] = {
    {
        "component",
        NULL,
        NULL,
        3,
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
        "window",
        NULL,
        NULL,
        4,
        window_testcases
    }
};

int main(int argc, char *argv[]) {
    return bake_test_run("siengine.test", argc, argv, suites, 3);
}
