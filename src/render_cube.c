#include "render_internal.h"
#include <SDL3/SDL_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline bool ensure_instance_cpu_capacity(SICubeRenderState *state, uint32_t needed) {
    if (needed <= state->instance_capacity) {
        return true;
    }

    uint32_t capacity = state->instance_capacity ? state->instance_capacity : 64;
    while (capacity < needed) {
        capacity *= 2;
    }

    SIInstanceData *instances = realloc(state->instances, sizeof(SIInstanceData) * capacity);
    if (instances == NULL) {
        fprintf(stderr, "siengine: failed to grow cube instance CPU buffer\n");
        return false;
    }

    state->instances = instances;
    state->instance_capacity = capacity;
    return true;
}

static inline bool ensure_instance_gpu_capacity() {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SICubeRenderState *state = &ecs_resource(SIRenderState)->cubes;
    uint32_t size = sizeof(SIInstanceData) * state->instance_capacity;

    if (state->instance_buffer != NULL && state->instance_transfer != NULL &&
        state->instance_gpu_capacity >= state->instance_capacity) {
        return true;
    }

    if (state->instance_buffer != NULL) {
        SDL_ReleaseGPUBuffer(engine->primary_gpu, state->instance_buffer);
        state->instance_buffer = NULL;
    }
    if (state->instance_transfer != NULL) {
        SDL_ReleaseGPUTransferBuffer(engine->primary_gpu, state->instance_transfer);
        state->instance_transfer = NULL;
    }

    state->instance_gpu_capacity = 0;
    state->instance_buffer = SDL_CreateGPUBuffer(
        engine->primary_gpu,
        &(SDL_GPUBufferCreateInfo){ .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = size }
    );
    state->instance_transfer = SDL_CreateGPUTransferBuffer(
        engine->primary_gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = size,
        }
    );
    if (state->instance_buffer == NULL || state->instance_transfer == NULL) {
        fprintf(stderr, "siengine: failed to grow cube instance GPU buffers: %s\n", SDL_GetError());
        return false;
    }

    state->instance_gpu_capacity = state->instance_capacity;
    return true;
}

void sirender_extract_cube_instances(ecs_iter_t *it) {
    SICubeRenderState *state = &ecs_resource(SIRenderState)->cubes;
    SIPosition3d *positions = ecs_field(it, 0);
    SIRotation3d *rotations = ecs_field(it, 1);
    SIScale3d *scales = ecs_field(it, 2);
    SIColor *colors = ecs_field(it, 3);

    if (!ensure_instance_cpu_capacity(state, state->instance_count + it->count)) {
        return;
    }

    for (uint32_t i = 0; i < it->count; i++) {
        SIInstanceData *instance = &state->instances[state->instance_count++];
        instance->model = si_mat4_model(positions[i], rotations[i], scales[i]);
        instance->color[0] = colors[i].r;
        instance->color[1] = colors[i].g;
        instance->color[2] = colors[i].b;
        instance->color[3] = colors[i].a;
    }
}

void sirender_upload_cube_instances(ecs_iter_t *it) {
    SIRenderState *render = ecs_resource(SIRenderState);
    SIRenderFrame *frame = &render->frame;
    SICubeRenderState *state = &render->cubes;
    if (state->instance_count == 0) {
        return;
    }
    if (!ensure_instance_gpu_capacity()) {
        return;
    }

    uint32_t size = sizeof(SIInstanceData) * state->instance_count;
    void *mapped = SDL_MapGPUTransferBuffer(
        ecs_resource(SIEngineCtx)->primary_gpu,
        state->instance_transfer,
        true
    );
    if (mapped == NULL) {
        fprintf(
            stderr,
            "siengine: SDL_MapGPUTransferBuffer(instance) failed: %s\n",
            SDL_GetError()
        );
        return;
    }

    memcpy(mapped, state->instances, size);
    SDL_UnmapGPUTransferBuffer(ecs_resource(SIEngineCtx)->primary_gpu, state->instance_transfer);

    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(frame->cmd);
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = state->instance_transfer,
                                          .offset = 0 },
        &(SDL_GPUBufferRegion){ .buffer = state->instance_buffer, .offset = 0, .size = size },
        true
    );
    SDL_EndGPUCopyPass(copy);
    state->instances_uploaded = true;
}

void sicube_draw_instances(SDL_GPURenderPass *pass, SIMat4 view_projection) {
    SIRenderState *render = ecs_resource(SIRenderState);
    SIRenderFrame *frame = &render->frame;
    SICubeRenderState *state = &render->cubes;
    if (state->instance_count == 0 || !state->instances_uploaded) {
        return;
    }

    SDL_GPUBufferBinding vertex_bindings[] = {
        { .buffer = state->vertex_buffer },
        { .buffer = state->instance_buffer },
    };
    SDL_GPUBufferBinding index_binding = { .buffer = state->index_buffer };
    SIVertexUniforms vertex_uniforms = {
        .view_projection = view_projection,
    };

    SDL_PushGPUVertexUniformData(frame->cmd, 0, &vertex_uniforms, sizeof(vertex_uniforms));
    SDL_BindGPUVertexBuffers(pass, 0, vertex_bindings, 2);
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, 36, state->instance_count, 0, 0, 0);
}

void sicube_render_state_shutdown() {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SICubeRenderState *state = &ecs_resource(SIRenderState)->cubes;

    free(state->instances);
    state->instances = NULL;
    state->instance_count = 0;
    state->instance_capacity = 0;
    state->instance_gpu_capacity = 0;

    if (state->pipeline != NULL) {
        SDL_ReleaseGPUGraphicsPipeline(engine->primary_gpu, state->pipeline);
        state->pipeline = NULL;
    }
    if (state->vertex_buffer != NULL) {
        SDL_ReleaseGPUBuffer(engine->primary_gpu, state->vertex_buffer);
        state->vertex_buffer = NULL;
    }
    if (state->index_buffer != NULL) {
        SDL_ReleaseGPUBuffer(engine->primary_gpu, state->index_buffer);
        state->index_buffer = NULL;
    }
    if (state->instance_buffer != NULL) {
        SDL_ReleaseGPUBuffer(engine->primary_gpu, state->instance_buffer);
        state->instance_buffer = NULL;
    }
    if (state->instance_transfer != NULL) {
        SDL_ReleaseGPUTransferBuffer(engine->primary_gpu, state->instance_transfer);
        state->instance_transfer = NULL;
    }
}
