#include "render_internal.h"
#include "siecs.h"
#include <math.h>
#include <stdlib.h>

void sirender_begin_frame(ecs_iter_t *it) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;

    frame->cmd = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
}

void sirender_extract(ecs_iter_t *it) {
    (void)it;
    SIRenderState *render = ecs_resource(SIRenderState);
    render->view_count = 0;
    render->queue.count = 0;

    ecs_iter_t cameras = ecs_query_iter(render->camera_query);
    while (ecs_iter_next(&cameras)) {
        SICamera2D *camera = ecs_field(&cameras, 0);
        SIWorldTransform2D *transform = ecs_field(&cameras, 1);
        SICameraViewport *viewport = ecs_field(&cameras, 2);
        SIVirtualResolution *virtual_resolution = ecs_field(&cameras, 3);

        for (uint32_t i = 0; i < cameras.count; i++) {
            if (render->view_count == render->view_capacity) {
                render->view_capacity = render->view_capacity ? render->view_capacity * 2 : 4;
                render->views = realloc(
                    render->views,
                    render->view_capacity * sizeof(*render->views)
                );
            }

            float width = camera[i].viewport_width / camera[i].zoom;
            float height = camera[i].viewport_height / camera[i].zoom;
            float viewport_x = viewport ? viewport[i].x : 0.0f;
            float viewport_y = viewport ? viewport[i].y : 0.0f;
            float viewport_width = viewport && viewport[i].width ? viewport[i].width : 1.0f;
            float viewport_height = viewport && viewport[i].height ? viewport[i].height : 1.0f;

            render->views[render->view_count++] = (SIRenderView){
                .left = transform[i].x - width * 0.5f,
                .top = transform[i].y - height * 0.5f,
                .right = transform[i].x + width * 0.5f,
                .bottom = transform[i].y + height * 0.5f,
                .viewport_x = viewport_x,
                .viewport_y = viewport_y,
                .viewport_width = viewport_width,
                .viewport_height = viewport_height,
                .virtual_width = virtual_resolution ? virtual_resolution[i].width : 0,
                .virtual_height = virtual_resolution ? virtual_resolution[i].height : 0,
                .virtual_enabled = virtual_resolution != NULL,
                .pixel_perfect = virtual_resolution ? virtual_resolution[i].pixel_perfect : false,
            };
        }
    }

    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderView *view = &render->views[view_index];
        ecs_iter_t sprites = ecs_query_iter(render->sprite_query);

        while (ecs_iter_next(&sprites)) {
            SISprite *sprite = ecs_field(&sprites, 0);
            SIWorldTransform2D *transform = ecs_field(&sprites, 1);
            SIColor *colors = ecs_field(&sprites, 2);
            SISpriteFlip *flips = ecs_field(&sprites, 3);
            SIPivot *pivots = ecs_field(&sprites, 4);
            SIBlendMode *blends = ecs_field(&sprites, 5);
            SISpriteSheet *sheets = ecs_field(&sprites, 6);
            for (uint32_t i = 0; i < sprites.count; i++) {
                SITextureHandle texture = sprite[i].texture;
                uint32_t texture_width = 1;
                uint32_t texture_height = 1;
                uint32_t width = texture_width;
                uint32_t height = texture_height;
                uint32_t region_x = 0;
                uint32_t region_y = 0;
                SIFilterMode filter = SI_FILTER_NEAREST;

                siengine_texture_info(
                    texture,
                    NULL,
                    &texture_width,
                    &texture_height,
                    &filter
                );

                if (sheets) {
                    uint32_t column = sprite[i].frame_index % sheets[i].columns;
                    uint32_t row = sprite[i].frame_index / sheets[i].columns;
                    region_x = sheets[i].margin_x +
                               column * (sheets[i].frame_width + sheets[i].spacing_x);
                    region_y = sheets[i].margin_y +
                               row * (sheets[i].frame_height + sheets[i].spacing_y);
                    width = sheets[i].frame_width;
                    height = sheets[i].frame_height;
                } else {
                    width = texture_width;
                    height = texture_height;
                }

                float pivot_x = pivots ? pivots[i].x : 0.5f;
                float pivot_y = pivots ? pivots[i].y : 0.5f;
                float half_width = (float)width * fabsf(transform[i].scale_x) * 0.5f;
                float half_height = (float)height * fabsf(transform[i].scale_y) * 0.5f;

                if (!sirender_rect_visible(
                        view,
                        transform[i].x,
                        transform[i].y,
                        half_width * 2.0f,
                        half_height * 2.0f
                    )) {
                    continue;
                }

                if (render->queue.count == render->queue.capacity) {
                    render->queue.capacity = render->queue.capacity
                                                  ? render->queue.capacity * 2
                                                  : 256;
                    render->queue.commands = realloc(
                        render->queue.commands,
                        render->queue.capacity * sizeof(*render->queue.commands)
                    );
                }

                SIColor color = colors ? colors[i] : (SIColor){ 1, 1, 1, 1 };
                render->queue.commands[render->queue.count++] = (SIRenderCommand){
                    .entity = sprites.entities[i],
                    .layer = ecs_target_at(&sprites, Layer, i),
                    .view_index = view_index,
                    .texture = texture,
                    .filter = filter,
                    .blend = blends ? blends[i].value : SI_BLEND_NORMAL,
                    .x = transform[i].x,
                    .y = transform[i].y,
                    .rotation = transform[i].rotation,
                    .scale_x = transform[i].scale_x,
                    .scale_y = transform[i].scale_y,
                    .pivot_x = pivot_x,
                    .pivot_y = pivot_y,
                    .u0 = (float)region_x / (float)texture_width,
                    .v0 = (float)region_y / (float)texture_height,
                    .u1 = (float)(region_x + width) / (float)texture_width,
                    .v1 = (float)(region_y + height) / (float)texture_height,
                    .width = (float)width,
                    .height = (float)height,
                    .color = color,
                    .flip_x = flips ? flips[i].x : false,
                    .flip_y = flips ? flips[i].y : false,
                };
            }
        }
    }
}

void sirender_end_frame(ecs_iter_t *it) {
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;

    if (!frame->cmd)
        return;

    SDL_SubmitGPUCommandBuffer(frame->cmd);
    frame->cmd = NULL;
}

void sirender_frame_shutdown() {
    SIRenderFrame *frame = &ecs_resource(SIRenderState)->frame;

    frame->cmd = NULL;
}
