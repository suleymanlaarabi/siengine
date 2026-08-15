
/* A friendly warning from bake.test
 * ----------------------------------------------------------------------------
 * This file is generated. To add/remove testcases modify the 'project.json' of
 * the test project. ANY CHANGE TO THIS FILE IS LOST AFTER (RE)BUILDING!
 * ----------------------------------------------------------------------------
 */

#include <test.h>

// Testsuite 'scene2d'
void scene2d_world_transform_adds_spatial_components(void);
void scene2d_imports_siphysics_automatically(void);
void scene2d_world_transform_updates_multiple_entities(void);
void scene2d_camera_requires_spatial_components(void);
void scene2d_camera_add_initializes_defaults(void);
void scene2d_query_matches_enabled_cameras(void);
void scene2d_virtual_resolution_add_initializes_defaults(void);
void scene2d_child_of_keeps_native_parent_relation(void);
void scene2d_world_transform_follows_parent(void);
void scene2d_world_transform_updates_hierarchy_by_depth(void);
void scene2d_default_layers_are_ordered(void);
void scene2d_sprite_requires_spatial_components(void);
void scene2d_shapes_require_transform_and_default_layer(void);
void scene2d_sprite_gets_default_world_layer(void);
void scene2d_explicit_layer_overrides_default(void);
void scene2d_sprite_sheet_describes_grid(void);

// Testsuite 'assets'
void assets_texture_path_uses_asset_root(void);
void assets_live_texture_is_released_during_ecs_fini(void);
void assets_animation_updates_sprite_frame(void);
void assets_texture_load_from_worker_is_queued(void);

// Testsuite 'render'
void render_queries_are_world_owned_across_cycles(void);
void render_sprite_defaults_are_components(void);
void render_extracts_once_for_multiple_views(void);
void render_extracts_postupdate_hierarchy_in_render_space(void);
void render_extracts_sheet_region_and_layer_order(void);
void render_extracts_colored_shapes(void);
void render_culls_shapes(void);

// Testsuite 'window'
void window_canvas_id_is_stored(void);

// Testsuite 'threading'
void threading_structural_mutations_are_deferred(void);
void threading_after_observes_structural_mutation(void);
void threading_read_read_resources_can_overlap(void);
void threading_read_write_resources_are_serialized(void);
void threading_write_write_resources_are_serialized(void);
void threading_unrelated_resources_can_overlap(void);
void threading_sirender_state_writers_are_serialized(void);

bake_test_case scene2d_testcases[] = {
    {
        "world_transform_adds_spatial_components",
        scene2d_world_transform_adds_spatial_components
    },
    {
        "imports_siphysics_automatically",
        scene2d_imports_siphysics_automatically
    },
    {
        "world_transform_updates_multiple_entities",
        scene2d_world_transform_updates_multiple_entities
    },
    {
        "camera_requires_spatial_components",
        scene2d_camera_requires_spatial_components
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
        "world_transform_updates_hierarchy_by_depth",
        scene2d_world_transform_updates_hierarchy_by_depth
    },
    {
        "default_layers_are_ordered",
        scene2d_default_layers_are_ordered
    },
    {
        "sprite_requires_spatial_components",
        scene2d_sprite_requires_spatial_components
    },
    {
        "shapes_require_transform_and_default_layer",
        scene2d_shapes_require_transform_and_default_layer
    },
    {
        "sprite_gets_default_world_layer",
        scene2d_sprite_gets_default_world_layer
    },
    {
        "explicit_layer_overrides_default",
        scene2d_explicit_layer_overrides_default
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
        "live_texture_is_released_during_ecs_fini",
        assets_live_texture_is_released_during_ecs_fini
    },
    {
        "animation_updates_sprite_frame",
        assets_animation_updates_sprite_frame
    },
    {
        "texture_load_from_worker_is_queued",
        assets_texture_load_from_worker_is_queued
    }
};

bake_test_case render_testcases[] = {
    {
        "queries_are_world_owned_across_cycles",
        render_queries_are_world_owned_across_cycles
    },
    {
        "sprite_defaults_are_components",
        render_sprite_defaults_are_components
    },
    {
        "extracts_once_for_multiple_views",
        render_extracts_once_for_multiple_views
    },
    {
        "extracts_postupdate_hierarchy_in_render_space",
        render_extracts_postupdate_hierarchy_in_render_space
    },
    {
        "extracts_sheet_region_and_layer_order",
        render_extracts_sheet_region_and_layer_order
    },
    {
        "extracts_colored_shapes",
        render_extracts_colored_shapes
    },
    {
        "culls_shapes",
        render_culls_shapes
    }
};

bake_test_case window_testcases[] = {
    {
        "canvas_id_is_stored",
        window_canvas_id_is_stored
    }
};

bake_test_case threading_testcases[] = {
    {
        "structural_mutations_are_deferred",
        threading_structural_mutations_are_deferred
    },
    {
        "after_observes_structural_mutation",
        threading_after_observes_structural_mutation
    },
    {
        "read_read_resources_can_overlap",
        threading_read_read_resources_can_overlap
    },
    {
        "read_write_resources_are_serialized",
        threading_read_write_resources_are_serialized
    },
    {
        "write_write_resources_are_serialized",
        threading_write_write_resources_are_serialized
    },
    {
        "unrelated_resources_can_overlap",
        threading_unrelated_resources_can_overlap
    },
    {
        "sirender_state_writers_are_serialized",
        threading_sirender_state_writers_are_serialized
    }
};


static bake_test_suite suites[] = {
    {
        "scene2d",
        NULL,
        NULL,
        16,
        scene2d_testcases
    },
    {
        "assets",
        NULL,
        NULL,
        4,
        assets_testcases
    },
    {
        "render",
        NULL,
        NULL,
        7,
        render_testcases
    },
    {
        "window",
        NULL,
        NULL,
        1,
        window_testcases
    },
    {
        "threading",
        NULL,
        NULL,
        7,
        threading_testcases
    }
};

int main(int argc, char *argv[]) {
    return bake_test_run("siengine.test", argc, argv, suites, 5);
}
