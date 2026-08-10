#include "engine_internal.h"
#include "siengine.h"
#include "siphysics.h"
#include <math.h>

ECS_CTOR(SIScale2D, { .x = 1.0f, .y = 1.0f });
ECS_CTOR(
    SIWorldTransform2D,
    { .x = 0.0f, .y = 0.0f, .rotation = 0.0f, .scale_x = 1.0f, .scale_y = 1.0f }
);
ECS_CTOR(SICameraViewport, { .width = 1.0f, .height = 1.0f });
ECS_CTOR(SICamera2D, { .zoom = 1.0f, .viewport_width = 320.0f, .viewport_height = 180.0f });
ECS_CTOR(SIVirtualResolution, { .width = 320, .height = 180, .pixel_perfect = true });

ECS_COMPONENT_DEFINE(SIScale2D, .ops = { .ctor = ecs_ctor_id(SIScale2D) });
ECS_COMPONENT_DEFINE(SIWorldTransform2D, .ops = { .ctor = ecs_ctor_id(SIWorldTransform2D) });
ECS_COMPONENT_DEFINE(SICamera2D, .ops = { .ctor = ecs_ctor_id(SICamera2D) });
ECS_COMPONENT_DEFINE(SICameraViewport, .ops = { .ctor = ecs_ctor_id(SICameraViewport) });
ECS_COMPONENT_DEFINE(SIVirtualResolution, .ops = { .ctor = ecs_ctor_id(SIVirtualResolution) });

void sitransform_update_parent(ecs_iter_t *it) {
    const Position *positions = ecs_field(it, 0);
    const Rotation *rotations = ecs_field(it, 1);
    const SIScale2D *scales = ecs_field(it, 2);
    SIWorldTransform2D *worlds = ecs_field(it, 3);
    const ecs_relation_target_t *parents = ecs_targets(it, ChildOf);
    for (uint32_t i = 0; i < it->count; i++) {
        SIWorldTransform2D *parent_world = ecs_try_get(parents[i].entity, SIWorldTransform2D);
        if (!parent_world) {
            worlds[i] = (SIWorldTransform2D){
                .x = positions[i].x,
                .y = positions[i].y,
                .rotation = rotations[i].angle,
                .scale_x = scales[i].x,
                .scale_y = scales[i].y,
            };
            continue;
        }
        float c = cosf(parent_world->rotation);
        float s = sinf(parent_world->rotation);
        float x = positions[i].x * parent_world->scale_x;
        float y = positions[i].y * parent_world->scale_y;
        worlds[i] = (SIWorldTransform2D){
            .x = parent_world->x + x * c - y * s,
            .y = parent_world->y + x * s + y * c,
            .rotation = parent_world->rotation + rotations[i].angle,
            .scale_x = parent_world->scale_x * scales[i].x,
            .scale_y = parent_world->scale_y * scales[i].y,
        };
    }
}

void sitransform_update_no_parent(ecs_iter_t *it) {
    const Position *positions = ecs_field(it, 0);
    const Rotation *rotations = ecs_field(it, 1);
    const SIScale2D *scales = ecs_field(it, 2);
    SIWorldTransform2D *worlds = ecs_field(it, 3);

    for (uint32_t i = 0; i < it->count; i++) {
        worlds[i] = (SIWorldTransform2D){
            .x = positions[i].x,
            .y = -positions[i].y,
            .rotation = rotations[i].angle,
            .scale_x = scales[i].x,
            .scale_y = scales[i].y,
        };
    }
}
