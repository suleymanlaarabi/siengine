#include "render_internal.h"
#include <math.h>
#include <stdlib.h>

uint32_t sirender_build_vertices(SIRenderState *render) {
    static const uint8_t corners[6][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 },
                                           { 0, 0 }, { 1, 1 }, { 0, 1 } };
    uint32_t required = 0;
    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderQueue *queue = &render->views[view_index].queue;
        for (uint32_t i = 0; i < queue->count; i++)
            required += queue->commands[i].primitive == SI_RENDER_TRIANGLE ? 3 : 6;
    }

    if (required > render->vertex_capacity) {
        render->vertex_capacity = render->vertex_capacity ? render->vertex_capacity * 2 : 1024;
        while (render->vertex_capacity < required)
            render->vertex_capacity *= 2;
        render->vertices =
            realloc(render->vertices, render->vertex_capacity * sizeof(*render->vertices));
    }

    uint32_t vertex_offset = 0;
    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderView *view = &render->views[view_index];
        SIRenderQueue *queue = &view->queue;
        view->vertex_offset = vertex_offset;

        for (uint32_t i = 0; i < queue->count; i++) {
            SIRenderCommand *command = &queue->commands[i];
            uint32_t vertex_count = command->primitive == SI_RENDER_TRIANGLE ? 3 : 6;
            command->vertex_offset = vertex_offset;
            float u0 = command->flip_x ? command->u1 : command->u0;
            float u1 = command->flip_x ? command->u0 : command->u1;
            float v0 = command->flip_y ? command->v1 : command->v0;
            float v1 = command->flip_y ? command->v0 : command->v1;
            float c = cosf(command->rotation);
            float s = sinf(command->rotation);

            for (uint32_t vertex = 0; vertex < vertex_count; vertex++) {
                float local_x;
                float local_y;
                float local_u;
                float local_v;
                if (command->primitive == SI_RENDER_TRIANGLE) {
                    static const float points[3][2] = {
                        { -0.5f, 0.5f },
                        { 0.5f, 0.5f },
                        { 0.0f, -0.5f },
                    };
                    local_x = points[vertex][0] * command->shape_a * command->scale_x;
                    local_y = points[vertex][1] * command->shape_b * command->scale_y;
                    local_u = 0.5f;
                    local_v = 0.5f;
                } else {
                    local_x = (corners[vertex][0] -
                               (command->primitive == SI_RENDER_SPRITE ? command->pivot_x : 0.5f)) *
                              command->width * command->scale_x;
                    local_y = (corners[vertex][1] -
                               (command->primitive == SI_RENDER_SPRITE ? command->pivot_y : 0.5f)) *
                              command->height * command->scale_y;
                    local_u = corners[vertex][0] ? u1 : u0;
                    local_v = corners[vertex][1] ? v1 : v0;
                }
                float world_x = command->x + local_x * c - local_y * s;
                float world_y = command->y + local_x * s + local_y * c;
                float clip_x = (world_x - view->left) / (view->right - view->left) * 2.0f - 1.0f;
                float clip_y = 1.0f - (world_y - view->top) / (view->bottom - view->top) * 2.0f;

                render->vertices[vertex_offset + vertex] = (SIRenderVertex){
                    .x = clip_x,
                    .y = clip_y,
                    .u = local_u,
                    .v = local_v,
                    .r = command->color.r,
                    .g = command->color.g,
                    .b = command->color.b,
                    .a = command->color.a,
                };
            }
            vertex_offset += vertex_count;
        }
    }
    return required;
}
