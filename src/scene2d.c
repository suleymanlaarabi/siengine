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

static void animation_on_add(ecs_entity_t entity, ecs_component_t component, void *value) {
    (void)component;
    (void)value;
    SIAnimationTimer *timer = ecs_get(entity, SIAnimationTimer);
    *timer = (SIAnimationTimer){ .playing = true };
}

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
    *ecs_get(entity, SIAnimationTimer) = (SIAnimationTimer){ .playing = true };
}

static void transform_on_add(ecs_entity_t entity, ecs_component_t component, void *value) {
    (void)entity;
    (void)component;
    SITransform2D *transform = value;
    *transform = (SITransform2D){ .scale_x = 1.0f, .scale_y = 1.0f };
}

static void world_transform_on_add(ecs_entity_t entity, ecs_component_t component, void *value) {
    (void)entity;
    (void)component;
    SIWorldTransform2D *transform = value;
    *transform = (SIWorldTransform2D){ .scale_x = 1.0f, .scale_y = 1.0f };
}

static void camera_viewport_on_add(ecs_entity_t entity, ecs_component_t component, void *value) {
    (void)entity;
    (void)component;
    SICameraViewport *viewport = value;
    *viewport = (SICameraViewport){ .width = 1.0f, .height = 1.0f };
}

static void sprite_on_add(ecs_entity_t entity, ecs_component_t component, void *value) {
    (void)component;
    (void)value;
    if (!ecs_has_relation(entity, Layer))
        ecs_relate(entity, Layer, SILayerActors);
}

ECS_COMPONENT_DEFINE(SITransform2D, .on_add = transform_on_add);
ECS_COMPONENT_DEFINE(SIWorldTransform2D, .on_add = world_transform_on_add);
ECS_COMPONENT_DEFINE(SICamera2D);
ECS_COMPONENT_DEFINE(SIActiveCamera);
ECS_COMPONENT_DEFINE(SICameraViewport, .on_add = camera_viewport_on_add);
ECS_COMPONENT_DEFINE(SIVirtualResolution);
ECS_COMPONENT_DEFINE(SIColor);
ECS_COMPONENT_DEFINE(SISprite, .on_add = sprite_on_add);
ECS_COMPONENT_DEFINE(SISpriteSheet);
ECS_COMPONENT_DEFINE(SISpriteFlip);
ECS_COMPONENT_DEFINE(SIPivot);
ECS_COMPONENT_DEFINE(SIBlendMode);
ECS_COMPONENT_DEFINE(SIAnimation, .on_add = animation_on_add, .on_set = animation_on_set);
ECS_COMPONENT_DEFINE(SIAnimationTimer);

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
    SITransform2D *restrict local = ecs_field(it, 0);
    SIWorldTransform2D *restrict world = ecs_field(it, 1);

    memcpy(world, local, sizeof(SITransform2D));
}

static void update_animations(ecs_iter_t *it) {
    SISprite *sprites = ecs_field(it, 0);
    SIAnimation *animations = ecs_field(it, 1);
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
    ECS_COMPONENT_REGISTER(SITransform2D);
    ECS_COMPONENT_REGISTER(SIWorldTransform2D);
    ECS_COMPONENT_REGISTER(SICamera2D);
    ECS_COMPONENT_REGISTER(SIActiveCamera);
    ECS_COMPONENT_REGISTER(SICameraViewport);
    ECS_COMPONENT_REGISTER(SIVirtualResolution);
    ECS_COMPONENT_REGISTER(SIColor);
    ECS_COMPONENT_REGISTER(SISprite);
    ECS_COMPONENT_REGISTER(SISpriteSheet);
    ECS_COMPONENT_REGISTER(SISpriteFlip);
    ECS_COMPONENT_REGISTER(SIPivot);
    ECS_COMPONENT_REGISTER(SIBlendMode);
    ECS_COMPONENT_REGISTER(SIAnimation);
    ECS_COMPONENT_REGISTER(SIAnimationTimer);

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

    ecs_with(ecs_id(SITransform2D), ecs_id(SIWorldTransform2D));
    ecs_with(ecs_id(SICamera2D), ecs_id(SITransform2D));
    ecs_with(ecs_id(SICamera2D), ecs_id(SIActiveCamera));
    ecs_with(ecs_id(SICamera2D), ecs_id(SICameraViewport));
    ecs_with(ecs_id(SISprite), ecs_id(SITransform2D));
    ecs_with(ecs_id(SIAnimation), ecs_id(SISprite));
    ecs_with(ecs_id(SIAnimation), ecs_id(SIAnimationTimer));

    ecs_system_id_t update_no_parent = ecs_system({
        .name = "UpdateWorldTransformsWithoutParent",
        .phase = EcsOnUpdate,
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
        .phase = EcsOnUpdate,
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
