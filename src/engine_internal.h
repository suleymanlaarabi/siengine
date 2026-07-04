#ifndef SIENGINE_INTERNAL_H
#define SIENGINE_INTERNAL_H

#include "siecs.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

typedef struct SIDepthTarget {
    SDL_Window *window;
    SDL_GPUTexture *texture;
    uint32_t width;
    uint32_t height;
} SIDepthTarget;

typedef struct SIInstanceData SIInstanceData;

ECS_RESOURCE_DECLARE(SIEngineCtx, {
    SDL_GPUDevice *primary_gpu;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUGraphicsPipeline *cube_pipeline;
    SDL_GPUBuffer *cube_vertex_buffer;
    SDL_GPUBuffer *cube_index_buffer;
    SDL_GPUBuffer *cube_instance_buffer;
    SDL_GPUTransferBuffer *cube_instance_transfer;
    SDL_GPUTextureFormat depth_format;
    SIDepthTarget depth_targets[8];
    uint32_t depth_target_count;
    SIInstanceData *instances;
    uint32_t instance_count;
    uint32_t instance_capacity;
    ecs_query_id_t cube_query;
    ecs_query_id_t camera_query;
});

ECS_COMPONENT_DECLARE(SIWindowHandle, {
    ptr handle;
    uint32_t width;
    uint32_t height;
});

ECS_MODULE_DECLARE(sitransform, {});

void siwindow_register(ecs_world_t *world);
void sirender_register(ecs_world_t *world);
void sirender_shutdown(ecs_world_t *world, SIEngineCtx *ctx);

#endif
