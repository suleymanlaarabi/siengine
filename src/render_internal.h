#ifndef SIENGINE_RENDER_INTERNAL_H
#define SIENGINE_RENDER_INTERNAL_H

#include "engine_internal.h"
#include "assets_internal.h"
#include "siengine.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    ecs_entity_t layer;
    SDL_GPUTexture *gpu_texture;
    SIFilterMode filter;
    SIBlendModeValue blend;
    float x;
    float y;
    float rotation;
    float scale_x;
    float scale_y;
    float pivot_x;
    float pivot_y;
    float u0;
    float v0;
    float u1;
    float v1;
    float width;
    float height;
    SIColor color;
    bool flip_x;
    bool flip_y;
} SIRenderCommand;

typedef struct {
    SIRenderCommand *commands;
    uint32_t count;
    uint32_t capacity;
} SIRenderQueue;

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
    SIRenderQueue queue;
    uint32_t vertex_offset;
} SIRenderView;

typedef struct {
    float x, y;
    float u, v;
    float r, g, b, a;
} SIRenderVertex;

ECS_RESOURCE_DECLARE(SIRenderState, {
    SDL_GPUCommandBuffer *cmd;
    SIRenderView *views;
    uint32_t view_count;
    uint32_t view_capacity;
    SIRenderVertex *vertices;
    uint32_t vertex_count;
    uint32_t vertex_capacity;
    SDL_GPUShader *vertex_shader;
    SDL_GPUShader *fragment_shader;
    SDL_GPUGraphicsPipeline *pipelines[3];
    SDL_GPUSampler *samplers[2];
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUTransferBuffer *transfer_buffer;
    uint32_t gpu_vertex_capacity;
    ecs_query_id_t camera_query;
    ecs_query_id_t sprite_query;
    ecs_system_id_t extract_system;
});

void sirender_begin_frame(ecs_iter_t *it);
void sirender_extract(ecs_iter_t *it);
void sirender_end_frame(ecs_iter_t *it);
void sirender_draw_window(ecs_iter_t *it);


#endif
