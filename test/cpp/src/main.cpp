
/* A friendly warning from bake.test
 * ----------------------------------------------------------------------------
 * This file is generated. To add/remove testcases modify the 'project.json' of
 * the test project. ANY CHANGE TO THIS FILE IS LOST AFTER (RE)BUILDING!
 * ----------------------------------------------------------------------------
 */

#include <cpp.h>

// Testsuite 'defaults'
void defaults_component_defaults(void);
void defaults_camera_defaults(void);
void defaults_sprite_default_layer(void);
void defaults_sprite_explicit_layer(void);
void defaults_asset_root_default(void);

bake_test_case defaults_testcases[] = {
    {
        "component_defaults",
        defaults_component_defaults
    },
    {
        "camera_defaults",
        defaults_camera_defaults
    },
    {
        "sprite_default_layer",
        defaults_sprite_default_layer
    },
    {
        "sprite_explicit_layer",
        defaults_sprite_explicit_layer
    },
    {
        "asset_root_default",
        defaults_asset_root_default
    }
};


static bake_test_suite suites[] = {
    {
        "defaults",
        NULL,
        NULL,
        5,
        defaults_testcases
    }
};

int main(int argc, char *argv[]) {
    return bake_test_run("siengine.test.cpp", argc, argv, suites, 1);
}
