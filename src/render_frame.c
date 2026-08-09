#include "render_internal.h"
#include "siecs.h"
#include "siengine.h"
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

static inline int compare_uint64(uint64_t a, uint64_t b) { return a < b ? -1 : a > b ? 1 : 0; }

static int compare_commands(const void *left_ptr, const void *right_ptr) {
    const SIRenderCommand *left = left_ptr;
    const SIRenderCommand *right = right_ptr;
    int result = compare_uint64(left->layer, right->layer);
    if (result)
        return result;
    if (left->pipeline != right->pipeline)
        return left->pipeline < right->pipeline ? -1 : 1;
    if (left->texture != right->texture)
        return compare_uint64(left->texture, right->texture);
    if (left->filter != right->filter)
        return left->filter < right->filter ? -1 : 1;
    if (left->blend != right->blend)
        return left->blend < right->blend ? -1 : 1;
    return compare_uint64(left->entity, right->entity);
}

static inline void
command_bounds(const SIRenderCommand *command, float *half_width, float *half_height) {
    float scale_x = fabsf(command->scale_x);
    float scale_y = fabsf(command->scale_y);

    if (command->primitive == SI_RENDER_CIRCLE) {
        float radius = command->shape_a * (scale_x > scale_y ? scale_x : scale_y);
        *half_width = radius;
        *half_height = radius;
        return;
    }

    if (command->primitive != SI_RENDER_TRIANGLE) {
        float c = fabsf(cosf(command->rotation));
        float s = fabsf(sinf(command->rotation));
        float width = command->width * scale_x * 0.5f;
        float height = command->height * scale_y * 0.5f;
        *half_width = c * width + s * height;
        *half_height = s * width + c * height;
        return;
    }

    float half_base = command->shape_a * 0.5f;
    float half_height_value = command->shape_b * 0.5f;
    float c = cosf(command->rotation);
    float s = sinf(command->rotation);
    float min_x = INFINITY;
    float max_x = -INFINITY;
    float min_y = INFINITY;
    float max_y = -INFINITY;
    const float points[3][2] = {
        { -half_base, half_height_value },
        { half_base, half_height_value },
        { 0.0f, -half_height_value },
    };
    for (uint32_t i = 0; i < 3; i++) {
        float x = points[i][0] * scale_x;
        float y = points[i][1] * scale_y;
        float world_x = x * c - y * s;
        float world_y = x * s + y * c;
        min_x = fminf(min_x, world_x);
        max_x = fmaxf(max_x, world_x);
        min_y = fminf(min_y, world_y);
        max_y = fmaxf(max_y, world_y);
    }
    *half_width = fmaxf(fabsf(min_x), fabsf(max_x));
    *half_height = fmaxf(fabsf(min_y), fabsf(max_y));
}

static void append_command(SIRenderState *render, const SIRenderCommand *command) {
    float half_width;
    float half_height;
    command_bounds(command, &half_width, &half_height);

    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderView *view = &render->views[view_index];
        if (!sirender_rect_visible(view, command->x, command->y, half_width, half_height))
            continue;

        ensure_commands(&view->queue);
        view->queue.commands[view->queue.count++] = *command;
    }
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

    ecs_iter_t renderables = ecs_query_iter(render->renderable_query);
    while (ecs_iter_next(&renderables)) {
        const SIWorldTransform2D *restrict transforms = ecs_field(&renderables, 0);
        const SIColor *restrict colors = ecs_field(&renderables, 1);
        const SIBlendMode *restrict blends = ecs_field(&renderables, 2);
        const SISprite *restrict sprites = ecs_field(&renderables, 3);
        const SISpriteFlip *restrict flips = ecs_field(&renderables, 4);
        const SIPivot *restrict pivots = ecs_field(&renderables, 5);
        const SISpriteSheet *restrict sheets = ecs_field(&renderables, 6);
        const SICircle *restrict circles = ecs_field(&renderables, 7);
        const SIRectangle *restrict rectangles = ecs_field(&renderables, 8);
        const SITriangle *restrict triangles = ecs_field(&renderables, 9);
        const ecs_entity_t layer = ecs_target_shared(&renderables, Layer);

        bool is_sheets_shared = ecs_field_is_shared(&renderables, 6);

        for (uint32_t i = 0; i < renderables.count; i++) {
            SIRenderCommand command = {
                .layer = layer,
                .entity = renderables.entities[i],
                .blend = blends[i].value,
                .x = transforms[i].x,
                .y = transforms[i].y,
                .rotation = transforms[i].rotation,
                .scale_x = transforms[i].scale_x,
                .scale_y = transforms[i].scale_y,
                .color = colors[i],
            };

            if (sprites) {
                SITextureHandle texture = sprites[i].texture;
                SITextureSlot *slot = siengine_texture_slot(texture);
                uint32_t width = slot->width;
                uint32_t height = slot->height;
                uint32_t region_x = 0;
                uint32_t region_y = 0;

                if (sheets) {
                    const SISpriteSheet *restrict sheet = is_sheets_shared ? sheets : &sheets[i];
                    uint32_t column = sprites[i].frame_index % sheet->columns;
                    uint32_t row = sprites[i].frame_index / sheet->columns;
                    region_x = sheet->margin_x + column * (sheet->frame_width + sheet->spacing_x);
                    region_y = sheet->margin_y + row * (sheet->frame_height + sheet->spacing_y);
                    width = sheet->frame_width;
                    height = sheet->frame_height;
                }

                command.primitive = SI_RENDER_SPRITE;
                command.pipeline = SI_RENDER_PIPELINE_SPRITE;
                command.texture = texture;
                command.gpu_texture = slot->gpu;
                command.filter = slot->filter;
                command.pivot_x = pivots[i].x;
                command.pivot_y = pivots[i].y;
                command.u0 = (float)region_x / slot->width;
                command.v0 = (float)region_y / slot->height;
                command.u1 = (float)(region_x + width) / slot->width;
                command.v1 = (float)(region_y + height) / slot->height;
                command.width = width;
                command.height = height;
                command.flip_x = flips[i].x;
                command.flip_y = flips[i].y;
            } else if (circles) {
                command.primitive = SI_RENDER_CIRCLE;
                command.pipeline = SI_RENDER_PIPELINE_CIRCLE;
                command.shape_a = circles[i].radius;
                command.width = circles[i].radius * 2.0f;
                command.height = circles[i].radius * 2.0f;
            } else if (rectangles) {
                command.primitive = SI_RENDER_RECTANGLE;
                command.pipeline = SI_RENDER_PIPELINE_SHAPE;
                command.width = rectangles[i].width;
                command.height = rectangles[i].height;
            } else if (triangles) {
                command.primitive = SI_RENDER_TRIANGLE;
                command.pipeline = SI_RENDER_PIPELINE_SHAPE;
                command.shape_a = triangles[i].base;
                command.shape_b = triangles[i].height;
            } else {
                continue;
            }

            append_command(render, &command);
        }
    }

    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderQueue *queue = &render->views[view_index].queue;
        if (queue->count > 1)
            qsort(queue->commands, queue->count, sizeof(*queue->commands), compare_commands);
    }
}

void sirender_end_frame(ecs_iter_t *it) {
    SIRenderState *render = ecs_resource(SIRenderState);

    if (!render->cmd)
        return;

    SDL_SubmitGPUCommandBuffer(render->cmd);
    render->cmd = NULL;
}
