#include "engine_internal.h"
#include "siecs.h"
#include "siengine.h"
#include "siphysics.h"
#include <string.h>

ECS_MODULE_DEFINE(siscene2d);
ECS_RELATION_DEFINE(
    Layer,
    {
        .storage = EcsRelationByTarget,
    }
);
ECS_RELATION_DEFINE(
    Material,
    {
        .storage = EcsRelationByTarget,
        .acyclic = true,
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
ecs_entity_t SI2DDefaultMaterial;

ECS_TAG_DEFINE(SIRenderable);

ECS_CTOR(SIColor, { 1, 1, 1, 1 });
ECS_CTOR(SISprite, { .frame_index = 0 });
ECS_CTOR(SICircle, { .radius = 1.0f });
ECS_CTOR(SIRectangle, { .width = 1.0f, .height = 1.0f });
ECS_CTOR(SITriangle, { .base = 1.0f, .height = 1.0f });
ECS_CTOR(SIPivot, { .x = 0.5f, .y = 0.5f });
ECS_CTOR(SIBlendMode, { .value = SI_BLEND_NORMAL });
ECS_COMPONENT_DEFINE(SIColor, .ops = { .ctor = ecs_ctor_id(SIColor) });
ECS_COMPONENT_DEFINE(SISprite, .ops = { .ctor = ecs_ctor_id(SISprite) });
ECS_COMPONENT_DEFINE(SICircle, .ops = { .ctor = ecs_ctor_id(SICircle) });
ECS_COMPONENT_DEFINE(SIRectangle, .ops = { .ctor = ecs_ctor_id(SIRectangle) });
ECS_COMPONENT_DEFINE(SITriangle, .ops = { .ctor = ecs_ctor_id(SITriangle) });
ECS_COMPONENT_DEFINE(SISpriteSheet, .inheritance = EcsInheritShared);
ECS_COMPONENT_DEFINE(SISpriteFlip);
ECS_COMPONENT_DEFINE(
    SIPivot,
    .inheritance = EcsInheritShared,
    .ops = { .ctor = ecs_ctor_id(SIPivot) }
);
ECS_COMPONENT_DEFINE(
    SIBlendMode,
    .inheritance = EcsInheritShared,
    .ops = { .ctor = ecs_ctor_id(SIBlendMode) }
);
ECS_COMPONENT_DEFINE(SIMaterial2D, .inheritance = EcsInheritShared);

ecs_entity_t Layers;

static ecs_entity_t create_layer(const char *name) {
    ecs_entity_t layer = ecs_new_no_reuse();
    ecs_set(layer, Name, { strdup(name) });
    ecs_relate(layer, ChildOf, Layers);
    return layer;
}

void siscene2d_import(const siscene2d_props_t *props) {
    ECS_MODULE_IMPORT(siphysics, {});

    ECS_RELATION_REGISTER(Layer);
    ECS_RELATION_REGISTER(Material);
    ECS_COMPONENT_REGISTER(
        SIRenderable,
        SIScale2D,
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
        SIAnimationTimer,
        SIMaterial2D
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

    ecs_with_relation(SIRenderable, Layer, SILayerWorld);
    ecs_with(SIMaterial2D, SISpriteSheet, SIPivot, SIBlendMode);

    SI2DDefaultMaterial = ecs_new();
    ecs_set(
        SI2DDefaultMaterial,
        SIMaterial2D,
        {
            .texture = SI_INVALID_HANDLE,
            .filter = SI_FILTER_NEAREST,
        }
    );

    ecs_with(SIWorldTransform2D, Position, Rotation, SIScale2D);
    ecs_with(SICamera2D, SIWorldTransform2D, SICameraViewport);
    ecs_with(SISprite, SIWorldTransform2D, SIColor, SISpriteFlip, SIRenderable);
    ecs_with(SICircle, SIWorldTransform2D, SIColor, SIRenderable);
    ecs_with(SIRectangle, SIWorldTransform2D, SIColor, SIRenderable);
    ecs_with(SITriangle, SIWorldTransform2D, SIColor, SIRenderable);
    ecs_with(SIAnimation, SISprite, SIAnimationTimer);

    ecs_system_id_t update_no_parent = ecs_system({
        .name = "UpdateWorldTransformsWithoutParent",
        .phase = EcsPostUpdate,
        .callback = sitransform_update_no_parent,
        .query = {
            .terms = {
                ecs_in(Position),
                ecs_in(Rotation),
                ecs_in(SIScale2D),
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
        .callback = sitransform_update_parent,
        .after = { update_no_parent },
        .query = {
            .terms = {
                ecs_in(Position),
                ecs_in(Rotation),
                ecs_in(SIScale2D),
                ecs_out(SIWorldTransform2D),
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
        .callback = sianimation_update,
        .query = {
            .terms = {
                ecs_inout(SISprite),
                ecs_in(SIAnimation),
                ecs_inout(SIAnimationTimer),
            },
        },
    });
}
