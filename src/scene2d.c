#include "engine_internal.h"
#include "siecs.h"
#include "siengine.h"
#include <math.h>
#include <string.h>

ECS_MODULE_DEFINE(siscene2d);
ECS_RELATION_DEFINE(
    Layer,
    {
        .storage = EcsRelationByTarget,
    }
);

ecs_entity_t SILayerBackground;
ecs_entity_t SILayerBackgroundDetail;
ecs_entity_t SILayerGround;
ecs_entity_t SILayerWorld;
ecs_entity_t SILayerActors;
ecs_entity_t SILayerEffects;
ecs_entity_t SILayerForeground;
ecs_entity_t SILayerOverlay;
ecs_entity_t SILayerDebug;
ecs_entity_t SILayerUI;

static void animation_on_set(
    ecs_entity_t entity,
    ecs_component_t component,
    const void *new_value,
    void *current_value
) {
    (void)component;
    (void)current_value;
    const SIAnimation *animation = new_value;
    ecs_get(entity, SISprite)->frame_index = animation->start_index;
}

ECS_CTOR(
    SITransform2D,
    { .x = 0.0f, .y = 0.0f, .rotation = 0.0f, .scale_x = 1.0f, .scale_y = 1.0f }
);
ECS_CTOR(
    SIWorldTransform2D,
    { .x = 0.0f, .y = 0.0f, .rotation = 0.0f, .scale_x = 1.0f, .scale_y = 1.0f }
);
ECS_CTOR(SICameraViewport, { .width = 1.0f, .height = 1.0f });
ECS_CTOR(
    SICamera2D,
    {
        .zoom = 1.0f,
        .viewport_width = 320.0f,
        .viewport_height = 180.0f,
    }
);
ECS_CTOR(SIVirtualResolution, { .width = 320, .height = 180, .pixel_perfect = true });

static void renderable_layer_on_add(ecs_entity_t entity, ecs_component_t component, void *value) {
    (void)component;
    (void)value;
    if (!ecs_has_relation(entity, Layer))
        ecs_relate(entity, Layer, SILayerWorld);
}

ECS_TAG_DEFINE(SIRenderable, .on_add = renderable_layer_on_add);

ECS_CTOR(SIColor, { 1, 1, 1, 1 });
ECS_CTOR(SISprite, { .texture = SI_INVALID_HANDLE });
ECS_CTOR(SICircle, { .radius = 1.0f });
ECS_CTOR(SIRectangle, { .width = 1.0f, .height = 1.0f });
ECS_CTOR(SITriangle, { .base = 1.0f, .height = 1.0f });
ECS_CTOR(SIPivot, { .x = 0.5f, .y = 0.5f });
ECS_CTOR(SIBlendMode, { .value = SI_BLEND_NORMAL });
ECS_CTOR(SIAnimationTimer, { .playing = true });

ECS_COMPONENT_DEFINE(SITransform2D, .ops = { .ctor = ecs_ctor_id(SITransform2D) });
ECS_COMPONENT_DEFINE(SIWorldTransform2D, .ops = { .ctor = ecs_ctor_id(SIWorldTransform2D) });
ECS_COMPONENT_DEFINE(SICamera2D, .ops = { .ctor = ecs_ctor_id(SICamera2D) });
ECS_COMPONENT_DEFINE(SICameraViewport, .ops = { .ctor = ecs_ctor_id(SICameraViewport) });
ECS_COMPONENT_DEFINE(SIVirtualResolution, .ops = { .ctor = ecs_ctor_id(SIVirtualResolution) });
ECS_COMPONENT_DEFINE(SIColor, .ops = { .ctor = ecs_ctor_id(SIColor) });
ECS_COMPONENT_DEFINE(SISprite, .ops = { .ctor = ecs_ctor_id(SISprite) });
ECS_COMPONENT_DEFINE(SICircle, .ops = { .ctor = ecs_ctor_id(SICircle) });
ECS_COMPONENT_DEFINE(SIRectangle, .ops = { .ctor = ecs_ctor_id(SIRectangle) });
ECS_COMPONENT_DEFINE(SITriangle, .ops = { .ctor = ecs_ctor_id(SITriangle) });
ECS_COMPONENT_DEFINE(SISpriteSheet, .inheritance = EcsInheritShared);
ECS_COMPONENT_DEFINE(SISpriteFlip);
ECS_COMPONENT_DEFINE(SIPivot, .ops = { .ctor = ecs_ctor_id(SIPivot) });
ECS_COMPONENT_DEFINE(SIBlendMode, .ops = { .ctor = ecs_ctor_id(SIBlendMode) });
ECS_COMPONENT_DEFINE(SIAnimation, .on_set = animation_on_set);
ECS_COMPONENT_DEFINE(SIAnimationTimer, .ops = { .ctor = ecs_ctor_id(SIAnimationTimer) });

ecs_entity_t Layers;

static ecs_entity_t create_layer(const char *name) {
    ecs_entity_t layer = ecs_new_no_reuse();
    ecs_set(layer, Name, { strdup(name) });
    ecs_relate(layer, ChildOf, Layers);
    return layer;
}

static void update_world_transforms_w_parent(ecs_iter_t *it) {
    const SITransform2D *restrict local = ecs_field(it, 0);
    SIWorldTransform2D *restrict world = ecs_field(it, 1);
    const ecs_relation_target_t *restrict parents = ecs_targets(it, ChildOf);

    for (uint32_t i = 0; i < it->count; i++) {
        SIWorldTransform2D *parent_world = ecs_try_get(parents[i].entity, SIWorldTransform2D);
        if (!parent_world) {
            world[i] = (SIWorldTransform2D){
                .x = local[i].x,
                .y = local[i].y,
                .rotation = local[i].rotation,
                .scale_x = local[i].scale_x,
                .scale_y = local[i].scale_y,
            };
            continue;
        }
        float c = cosf(parent_world->rotation);
        float s = sinf(parent_world->rotation);
        float x = local[i].x * parent_world->scale_x;
        float y = local[i].y * parent_world->scale_y;

        world[i] = (SIWorldTransform2D){
            .x = parent_world->x + x * c - y * s,
            .y = parent_world->y + x * s + y * c,
            .rotation = parent_world->rotation + local[i].rotation,
            .scale_x = parent_world->scale_x * local[i].scale_x,
            .scale_y = parent_world->scale_y * local[i].scale_y,
        };
    }
}

static void update_world_transforms_no_parent(ecs_iter_t *it) {
    const SITransform2D *restrict local = ecs_field(it, 0);
    SIWorldTransform2D *restrict world = ecs_field(it, 1);

    memcpy(world, local, it->count * sizeof(*world));
}

static void update_animations(ecs_iter_t *it) {
    SISprite *sprites = ecs_field(it, 0);
    const SIAnimation *restrict animations = ecs_field(it, 1);
    SIAnimationTimer *timers = ecs_field(it, 2);

    for (uint32_t i = 0; i < it->count; i++) {
        if (!timers[i].playing)
            continue;

        timers[i].elapsed += it->delta_time;
        while (timers[i].elapsed >= animations[i].frame_duration) {
            timers[i].elapsed -= animations[i].frame_duration;
            if (sprites[i].frame_index < animations[i].end_index) {
                sprites[i].frame_index++;
            } else if (animations[i].loop) {
                sprites[i].frame_index = animations[i].start_index;
            } else {
                sprites[i].frame_index = animations[i].end_index;
                timers[i].playing = false;
                break;
            }
        }
    }
}

void siscene2d_import(const siscene2d_props_t *props) {
    ECS_RELATION_REGISTER(Layer);
    ECS_COMPONENT_REGISTER(
        SIRenderable,
        SITransform2D,
        SIWorldTransform2D,
        SICamera2D,
        SICameraViewport,
        SIVirtualResolution,
        SIColor,
        SISprite,
        SICircle,
        SIRectangle,
        SITriangle,
        SISpriteSheet,
        SISpriteFlip,
        SIPivot,
        SIBlendMode,
        SIAnimation,
        SIAnimationTimer
    );

    Layers = ecs_new();
    ecs_set(Layers, Name, { strdup("Layers") });
    SILayerBackground = create_layer("Background");
    SILayerBackgroundDetail = create_layer("BackgroundDetail");
    SILayerGround = create_layer("Ground");
    SILayerWorld = create_layer("World");
    SILayerActors = create_layer("Actors");
    SILayerEffects = create_layer("Effects");
    SILayerForeground = create_layer("Foreground");
    SILayerOverlay = create_layer("Overlay");
    SILayerDebug = create_layer("Debug");
    SILayerUI = create_layer("UI");

    ecs_with(SITransform2D, SIWorldTransform2D);
    ecs_with(SICamera2D, SITransform2D, SICameraViewport);
    ecs_with(SISprite, SITransform2D, SIColor, SISpriteFlip, SIPivot, SIBlendMode, SIRenderable);
    ecs_with(SICircle, SITransform2D, SIColor, SIBlendMode, SIRenderable);
    ecs_with(SIRectangle, SITransform2D, SIColor, SIBlendMode, SIRenderable);
    ecs_with(SITriangle, SITransform2D, SIColor, SIBlendMode, SIRenderable);
    ecs_with(SIAnimation, SISprite, SIAnimationTimer);

    ecs_system_id_t update_no_parent = ecs_system({
        .name = "UpdateWorldTransformsWithoutParent",
        .phase = EcsPostUpdate,
        .callback = update_world_transforms_no_parent,
        .query = {
            .terms = {
                ecs_in(SITransform2D),
                ecs_out(SIWorldTransform2D),
            },
            .relations = {
                ecs_not_rel(ChildOf),
            },
        },
    });

    ecs_system({
        .name = "UpdateWorldTransformsWithParent",
        .phase = EcsPostUpdate,
        .callback = update_world_transforms_w_parent,
        .after = { update_no_parent },
        .query = {
            .terms = {
                ecs_in(SITransform2D),
                ecs_out(SIWorldTransform2D)
            },
            .relations = {
                ecs_rel(ChildOf),
            },
            .order_by = ecs_order_by_depth(ChildOf),
        },
    });

    ecs_system({
        .name = "UpdateAnimations",
        .phase = EcsOnUpdate,
        .callback = update_animations,
        .query = {
            .terms = {
                ecs_inout(SISprite),
                ecs_in(SIAnimation),
                ecs_inout(SIAnimationTimer),
            },
        },
    });
}
