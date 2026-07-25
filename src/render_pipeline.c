#include "render_internal.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    SDL_GPUShader *vertex;
    SDL_GPUShader *fragment;
} SIShaderPair;

static const SICubeVertex CUBE_VERTICES[] = {
    { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
    { -0.5f, -0.5f, 0.5f },  { 0.5f, -0.5f, 0.5f },  { 0.5f, 0.5f, 0.5f },  { -0.5f, 0.5f, 0.5f },
};

static const uint16_t CUBE_INDICES[] = {
    0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
    3, 6, 2, 3, 7, 6, 1, 2, 6, 1, 6, 5, 0, 4, 7, 0, 7, 3,
};

static void *load_file_from_project(const char *path, size_t *size) {
    void *data = SDL_LoadFile(path, size);
    if (data != NULL) {
        return data;
    }

    char parent_path[256];
    snprintf(parent_path, sizeof(parent_path), "../%s", path);
    return SDL_LoadFile(parent_path, size);
}

static SDL_GPUShader *load_shader(
    SDL_GPUDevice *gpu,
    const char *path,
    SDL_GPUShaderStage stage,
    uint32_t uniform_buffers
) {
    size_t code_size = 0;
    void *code = load_file_from_project(path, &code_size);
    if (code == NULL) {
        fprintf(stderr, "siengine: failed to load shader %s: %s\n", path, SDL_GetError());
        return NULL;
    }

    SDL_GPUShader *shader = SDL_CreateGPUShader(
        gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = code_size,
            .code = code,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = stage,
            .num_uniform_buffers = uniform_buffers,
        }
    );
    SDL_free(code);

    if (shader == NULL) {
        fprintf(stderr, "siengine: SDL_CreateGPUShader failed for %s: %s\n", path, SDL_GetError());
    }

    return shader;
}

static SIShaderPair load_cube_shaders(SDL_GPUDevice *gpu) {
    return (SIShaderPair){
        .vertex = load_shader(gpu, "shaders/cube.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1),
        .fragment = load_shader(gpu, "shaders/cube.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0),
    };
}

static void upload_cube_mesh(SIEngineCtx *engine, SICubeRenderState *state) {
    uint32_t vertex_size = sizeof(CUBE_VERTICES);
    uint32_t index_size = sizeof(CUBE_INDICES);
    uint32_t transfer_size = vertex_size + index_size;

    state->vertex_buffer = SDL_CreateGPUBuffer(
        engine->primary_gpu,
        &(SDL_GPUBufferCreateInfo){ .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = vertex_size }
    );
    state->index_buffer = SDL_CreateGPUBuffer(
        engine->primary_gpu,
        &(SDL_GPUBufferCreateInfo){ .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = index_size }
    );
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(
        engine->primary_gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = transfer_size,
        }
    );

    uint8_t *mapped = SDL_MapGPUTransferBuffer(engine->primary_gpu, transfer, false);
    memcpy(mapped, CUBE_VERTICES, vertex_size);
    memcpy(mapped + vertex_size, CUBE_INDICES, index_size);
    SDL_UnmapGPUTransferBuffer(engine->primary_gpu, transfer);

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer, .offset = 0 },
        &(SDL_GPUBufferRegion){ .buffer = state->vertex_buffer, .offset = 0, .size = vertex_size },
        false
    );
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer, .offset = vertex_size },
        &(SDL_GPUBufferRegion){ .buffer = state->index_buffer, .offset = 0, .size = index_size },
        false
    );
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(engine->primary_gpu, transfer);
}

void sicube_ensure_pipeline(SDL_GPUTextureFormat color_format) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SICubeRenderState *state = &ecs_resource(SIRenderState)->cubes;

    if (state->pipeline != NULL) {
        SDL_ReleaseGPUGraphicsPipeline(engine->primary_gpu, state->pipeline);
        state->pipeline = NULL;
    }
    if (state->vertex_buffer == NULL || state->index_buffer == NULL) {
        upload_cube_mesh(engine, state);
    }

    SIShaderPair shaders = load_cube_shaders(engine->primary_gpu);
    if (shaders.vertex == NULL || shaders.fragment == NULL) {
        if (shaders.vertex != NULL) {
            SDL_ReleaseGPUShader(engine->primary_gpu, shaders.vertex);
        }
        if (shaders.fragment != NULL) {
            SDL_ReleaseGPUShader(engine->primary_gpu, shaders.fragment);
        }
    }

    SDL_GPUVertexBufferDescription vertex_buffers[] = {
        {
            .slot = 0,
            .pitch = sizeof(SICubeVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        },
        {
            .slot = 1,
            .pitch = sizeof(SIInstanceData),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        },
    };
    SDL_GPUVertexAttribute vertex_attributes[] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(SICubeVertex, x),
        },
        {
            .location = 1,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(SIInstanceData, model) + sizeof(float) * 0,
        },
        {
            .location = 2,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(SIInstanceData, model) + sizeof(float) * 4,
        },
        {
            .location = 3,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(SIInstanceData, model) + sizeof(float) * 8,
        },
        {
            .location = 4,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(SIInstanceData, model) + sizeof(float) * 12,
        },
        {
            .location = 5,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(SIInstanceData, color),
        },
    };
    SDL_GPUColorTargetDescription color_targets[] = {
        { .format = color_format },
    };

    state->depth_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    state->pipeline = SDL_CreateGPUGraphicsPipeline(
        engine->primary_gpu,
        &(SDL_GPUGraphicsPipelineCreateInfo){
            .vertex_shader = shaders.vertex,
            .fragment_shader = shaders.fragment,
            .vertex_input_state =
                {
                    .vertex_buffer_descriptions = vertex_buffers,
                    .num_vertex_buffers = 2,
                    .vertex_attributes = vertex_attributes,
                    .num_vertex_attributes = 6,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state =
                {
                    .fill_mode = SDL_GPU_FILLMODE_FILL,
                    .cull_mode = SDL_GPU_CULLMODE_BACK,
                    .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
                    .enable_depth_clip = true,
                },
            .multisample_state = { .sample_count = SDL_GPU_SAMPLECOUNT_1 },
            .depth_stencil_state =
                {
                    .compare_op = SDL_GPU_COMPAREOP_LESS,
                    .enable_depth_test = true,
                    .enable_depth_write = true,
                },
            .target_info =
                {
                    .color_target_descriptions = color_targets,
                    .num_color_targets = 1,
                    .depth_stencil_format = state->depth_format,
                    .has_depth_stencil_target = true,
                },
        }
    );
    state->color_format = color_format;

    SDL_ReleaseGPUShader(engine->primary_gpu, shaders.vertex);
    SDL_ReleaseGPUShader(engine->primary_gpu, shaders.fragment);
}
