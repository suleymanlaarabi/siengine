#ifndef SIENGINE_RENDER_INTERNAL_H
#define SIENGINE_RENDER_INTERNAL_H

#include "engine_internal.h"
#include "siengine.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float x, y, z;
} SICubeVertex;

typedef struct {
    SIMat4 view_projection;
} SIVertexUniforms;

typedef struct SIInstanceData {
    SIMat4 model;
    float color[4];
} SIInstanceData;

typedef struct {
    ecs_entity_t entity;
    SIMat4 view;
    SICamera3d camera;
} SIRenderView;

typedef struct {
    SDL_Window *window;
    SDL_GPUTexture *texture;
    uint32_t width;
    uint32_t height;
} SIDepthTarget;

ECS_RESOURCE_DECLARE(SIRenderQueries, { ecs_query_id_t cameras; });

ECS_RESOURCE_DECLARE(SIRenderFrame, {
    SDL_GPUCommandBuffer *cmd;
    SIRenderView *views;
    uint32_t view_count;
    uint32_t view_capacity;
});

ECS_RESOURCE_DECLARE(SICubeRenderState, {
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    SDL_GPUBuffer *instance_buffer;
    SDL_GPUTransferBuffer *instance_transfer;
    SIInstanceData *instances;
    uint32_t instance_count;
    uint32_t instance_capacity;
    uint32_t instance_gpu_capacity;
    bool instances_uploaded;
    SDL_GPUTextureFormat color_format;
    SDL_GPUTextureFormat depth_format;
});

ECS_RESOURCE_DECLARE(SIWindowRenderState, {
    SIDepthTarget depth_targets[8];
    uint32_t depth_target_count;
});

void sirender_register_resources();

void sirender_begin_frame(ecs_iter_t *it);
void sirender_end_frame(ecs_iter_t *it);
void sirender_extract_views(ecs_iter_t *it);
void sirender_extract_cube_instances(ecs_iter_t *it);
void sirender_upload_cube_instances(ecs_iter_t *it);
void sirender_draw_windows(ecs_iter_t *it);

bool sicube_ensure_pipeline(SDL_GPUTextureFormat color_format);
void sicube_draw_instances(SDL_GPURenderPass *pass, SIMat4 view_projection);
void sicube_render_state_shutdown();

SDL_GPUTexture *siwindow_ensure_depth_target(SDL_Window *window, uint32_t width, uint32_t height);
void siwindow_render_state_shutdown();

void sirender_queries_shutdown();
void sirender_frame_shutdown();

#endif
