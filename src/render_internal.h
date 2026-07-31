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
    SIMat4 view;
    SICamera3d camera;
} SIRenderView;

typedef struct {
    SDL_GPUTexture *texture;
    uint32_t width;
    uint32_t height;
} SIDepthTarget;

typedef struct {
    SDL_GPUCommandBuffer *cmd;
    SIRenderView *views;
    uint32_t view_count;
    uint32_t view_capacity;
} SIRenderFrame;

typedef struct {
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
} SICubeRenderState;

typedef struct {
    SIDepthTarget depth_target;
} SIWindowRenderState;

ECS_RESOURCE_DECLARE(SIRenderState, {
    SIRenderFrame frame;
    SICubeRenderState cubes;
    SIWindowRenderState windows;
});

void sirender_begin_frame(ecs_iter_t *it);
void sirender_end_frame(ecs_iter_t *it);
void sirender_extract_views(ecs_iter_t *it);
void sirender_extract_cube_instances(ecs_iter_t *it);
void sirender_upload_cube_instances(ecs_iter_t *it);
void sirender_draw_window(ecs_iter_t *it);

void sicube_ensure_pipeline(SDL_GPUTextureFormat color_format);
void sicube_draw_instances(SDL_GPURenderPass *pass, SIMat4 view_projection);
void sicube_render_state_shutdown();

SDL_GPUTexture *siwindow_ensure_depth_target(uint32_t width, uint32_t height);
void siwindow_render_state_shutdown();

void sirender_frame_shutdown();

#endif
