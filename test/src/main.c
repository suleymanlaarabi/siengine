
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
void scene2d_camera_add_initializes_defaults(void);
void scene2d_query_matches_enabled_cameras(void);
void scene2d_virtual_resolution_add_initializes_defaults(void);
void scene2d_child_of_keeps_native_parent_relation(void);
void scene2d_world_transform_follows_parent(void);
void scene2d_default_layers_are_ordered(void);
void scene2d_sprite_requires_transform(void);
void scene2d_sprite_sheet_describes_grid(void);

// Testsuite 'assets'
void assets_texture_path_uses_asset_root(void);
void assets_animation_updates_sprite_frame(void);

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
        "camera_add_initializes_defaults",
        scene2d_camera_add_initializes_defaults
    },
    {
        "query_matches_enabled_cameras",
        scene2d_query_matches_enabled_cameras
    },
    {
        "virtual_resolution_add_initializes_defaults",
        scene2d_virtual_resolution_add_initializes_defaults
    },
    {
        "child_of_keeps_native_parent_relation",
        scene2d_child_of_keeps_native_parent_relation
    },
    {
        "world_transform_follows_parent",
        scene2d_world_transform_follows_parent
    },
    {
        "default_layers_are_ordered",
        scene2d_default_layers_are_ordered
    },
    {
        "sprite_requires_transform",
        scene2d_sprite_requires_transform
    },
    {
        "sprite_sheet_describes_grid",
        scene2d_sprite_sheet_describes_grid
    }
};

bake_test_case assets_testcases[] = {
    {
        "texture_path_uses_asset_root",
        assets_texture_path_uses_asset_root
    },
    {
        "animation_updates_sprite_frame",
        assets_animation_updates_sprite_frame
    }
};


static bake_test_suite suites[] = {
    {
        "scene2d",
        NULL,
        NULL,
        10,
        scene2d_testcases
    },
    {
        "assets",
        NULL,
        NULL,
        2,
        assets_testcases
    }
};

int main(int argc, char *argv[]) {
    return bake_test_run("siengine.test", argc, argv, suites, 2);
}
