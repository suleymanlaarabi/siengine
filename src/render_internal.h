#ifndef SIENGINE_RENDER_INTERNAL_H
#define SIENGINE_RENDER_INTERNAL_H

#include "assets_internal.h"
#include "backend.h"
#include "engine_internal.h"
#include "sicore.h"
#include "siengine.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SI_PIPELINE_SPRITE,
    SI_PIPELINE_SHAPE,
    SI_PIPELINE_CIRCLE,
    SI_PIPELINE_COUNT,
} SIRenderPipeline;

typedef enum {
    SI_GEOMETRY_QUAD,
    SI_GEOMETRY_TRIANGLE,
    SI_GEOMETRY_COUNT,
} SIRenderGeometry;

typedef struct {
    float x;
    float y;
    float rotation;
    float scale_x;
    float scale_y;
    float width;
    float height;
    SIColor color;
    float frame_index;
    float flip_x;
    float flip_y;
} SIInstance2D;

typedef struct {
    ecs_entity_t layer;
    ecs_entity_t material;
    SITextureHandle texture;
    uint64_t gpu_texture;
    uint32_t texture_width;
    uint32_t texture_height;
    SIFilterMode filter;
    SIBlendModeValue blend;
    SIRenderPipeline pipeline;
    SIRenderGeometry geometry;
    float pivot_x;
    float pivot_y;
    SISpriteSheet sheet;
    bool has_sheet;
    uint32_t instance_offset;
    uint32_t instance_count;
} SIRenderBatch;

typedef struct {
    float left;
    float top;
    float right;
    float bottom;
    float viewport_x;
    float viewport_y;
    float viewport_width;
    float viewport_height;
    uint32_t virtual_width;
    uint32_t virtual_height;
    bool virtual_enabled;
    bool pixel_perfect;
} SIRenderView;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} SIRenderViewport;

static inline SIRenderViewport sirender_viewport_rect(
    const SIRenderView *view,
    uint32_t pixel_width,
    uint32_t pixel_height
) {
    float x = view->viewport_x * pixel_width;
    float y = view->viewport_y * pixel_height;
    float width = view->viewport_width * pixel_width;
    float height = view->viewport_height * pixel_height;
    if (view->virtual_enabled) {
        float scale_x = width / (float)view->virtual_width;
        float scale_y = height / (float)view->virtual_height;
        float scale = scale_x < scale_y ? scale_x : scale_y;
        if (view->pixel_perfect)
            scale = floorf(scale);
        float output_width = view->virtual_width * scale;
        float output_height = view->virtual_height * scale;
        x += (width - output_width) * 0.5f;
        y += (height - output_height) * 0.5f;
        width = output_width;
        height = output_height;
    }
    return (SIRenderViewport){ x, y, width, height };
}

ECS_RESOURCE_DECLARE(SIRenderState, {
    sicore_vec_t views;
    sicore_vec_t batches;
    sicore_vec_t instances;
    ecs_query_id_t camera_query;
    ecs_query_id_t renderable_query;
});

void sirender_begin_frame(ecs_iter_t *it);
void sirender_extract(ecs_iter_t *it);
void sirender_end_frame(ecs_iter_t *it);
void sirender_draw_window(ecs_iter_t *it);
void sirender_register(void);
void sirender_shutdown(void);

#endif
