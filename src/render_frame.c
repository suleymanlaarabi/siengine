#include "render_internal.h"
#include "siecs.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void sirender_begin_frame(ecs_iter_t *it) {
    ecs_resource(SIRenderState)->cmd =
        SDL_AcquireGPUCommandBuffer(ecs_resource(SIEngineCtx)->primary_gpu);
}

static void ensure_views(SIRenderState *render, uint32_t count) {
    if (count <= render->view_capacity)
        return;

    uint32_t previous = render->view_capacity;
    render->view_capacity = previous ? previous * 2 : 4;
    while (render->view_capacity < count)
        render->view_capacity *= 2;
    render->views = realloc(render->views, render->view_capacity * sizeof(*render->views));
    memset(
        render->views + previous,
        0,
        (render->view_capacity - previous) * sizeof(*render->views)
    );
}

static void ensure_commands(SIRenderQueue *queue) {
    if (queue->count < queue->capacity)
        return;

    queue->capacity = queue->capacity ? queue->capacity * 2 : 256;
    queue->commands = realloc(queue->commands, queue->capacity * sizeof(*queue->commands));
}

static inline bool sirender_rect_visible(
    const SIRenderView *view,
    float x,
    float y,
    float half_width,
    float half_height
) {
    return x + half_width >= view->left && x - half_width <= view->right &&
           y + half_height >= view->top && y - half_height <= view->bottom;
}

void sirender_extract(ecs_iter_t *it) {
    (void)it;
    SIRenderState *render = ecs_resource(SIRenderState);
    render->view_count = 0;

    ecs_iter_t cameras = ecs_query_iter(render->camera_query);
    while (ecs_iter_next(&cameras)) {
        const SICamera2D *restrict camera = ecs_field(&cameras, 0);
        const SIWorldTransform2D *restrict transform = ecs_field(&cameras, 1);
        const SICameraViewport *restrict viewport = ecs_field(&cameras, 2);
        const SIVirtualResolution *restrict virtual_resolution = ecs_field(&cameras, 3);

        for (uint32_t i = 0; i < cameras.count; i++) {
            ensure_views(render, render->view_count + 1);
            SIRenderView *view = &render->views[render->view_count++];
            SIRenderQueue queue = view->queue;
            float width = camera[i].viewport_width / camera[i].zoom;
            float height = camera[i].viewport_height / camera[i].zoom;

            *view = (SIRenderView){
                .left = transform[i].x - width * 0.5f,
                .top = transform[i].y - height * 0.5f,
                .right = transform[i].x + width * 0.5f,
                .bottom = transform[i].y + height * 0.5f,
                .viewport_x = viewport[i].x,
                .viewport_y = viewport[i].y,
                .viewport_width = viewport[i].width,
                .viewport_height = viewport[i].height,
                .virtual_width = virtual_resolution ? virtual_resolution[i].width : 0,
                .virtual_height = virtual_resolution ? virtual_resolution[i].height : 0,
                .virtual_enabled = virtual_resolution != NULL,
                .pixel_perfect = virtual_resolution ? virtual_resolution[i].pixel_perfect : false,
                .queue = queue,
            };
            view->queue.count = 0;
        }
    }

    ecs_iter_t sprites = ecs_query_iter(render->sprite_query);
    while (ecs_iter_next(&sprites)) {
        const SISprite *restrict sprite = ecs_field(&sprites, 0);
        const SIWorldTransform2D *restrict transform = ecs_field(&sprites, 1);
        const SIColor *restrict colors = ecs_field(&sprites, 2);
        const SISpriteFlip *restrict flips = ecs_field(&sprites, 3);
        const SIPivot *restrict pivots = ecs_field(&sprites, 4);
        const SIBlendMode *restrict blends = ecs_field(&sprites, 5);
        const SISpriteSheet *restrict sheets = ecs_field(&sprites, 6);
        const ecs_entity_t layer = ecs_target_shared(&sprites, Layer);

        for (uint32_t i = 0; i < sprites.count; i++) {
            SITextureHandle texture = sprite[i].texture;
            SITextureSlot *slot = siengine_texture_slot(texture);
            uint32_t width = slot->width;
            uint32_t height = slot->height;
            uint32_t region_x = 0;
            uint32_t region_y = 0;

            if (sheets) {
                uint32_t column = sprite[i].frame_index % sheets[i].columns;
                uint32_t row = sprite[i].frame_index / sheets[i].columns;
                region_x =
                    sheets[i].margin_x + column * (sheets[i].frame_width + sheets[i].spacing_x);
                region_y =
                    sheets[i].margin_y + row * (sheets[i].frame_height + sheets[i].spacing_y);
                width = sheets[i].frame_width;
                height = sheets[i].frame_height;
            }

            float half_width = width * fabsf(transform[i].scale_x) * 0.5f;
            float half_height = height * fabsf(transform[i].scale_y) * 0.5f;
            SIRenderCommand command = {
                .layer = layer,
                .gpu_texture = slot->gpu,
                .filter = slot->filter,
                .blend = blends[i].value,
                .x = transform[i].x,
                .y = transform[i].y,
                .rotation = transform[i].rotation,
                .scale_x = transform[i].scale_x,
                .scale_y = transform[i].scale_y,
                .pivot_x = pivots[i].x,
                .pivot_y = pivots[i].y,
                .u0 = (float)region_x / slot->width,
                .v0 = (float)region_y / slot->height,
                .u1 = (float)(region_x + width) / slot->width,
                .v1 = (float)(region_y + height) / slot->height,
                .width = (float)width,
                .height = (float)height,
                .color = colors[i],
                .flip_x = flips[i].x,
                .flip_y = flips[i].y,
            };

            for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
                SIRenderView *view = &render->views[view_index];
                if (!sirender_rect_visible(view, command.x, command.y, half_width, half_height))
                    continue;

                ensure_commands(&view->queue);
                view->queue.commands[view->queue.count++] = command;
            }
        }
    }
}

void sirender_end_frame(ecs_iter_t *it) {
    SIRenderState *render = ecs_resource(SIRenderState);

    if (!render->cmd)
        return;

    SDL_SubmitGPUCommandBuffer(render->cmd);
    render->cmd = NULL;
}
