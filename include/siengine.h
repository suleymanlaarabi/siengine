#ifndef SIENGINE_H
#define SIENGINE_H

/* This generated file contains includes for project dependencies */
#include "siecs.h"
#include "siengine/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEG2RAD(deg) ((deg) * 0.01745329251994329576923690768489)

ECS_MODULE_DECLARE(siengine, {});
ECS_RESOURCE_DECLARE(SIWindow, {
    char title[128];
    uint32_t width;
    uint32_t height;
    bool resizable;
    bool vsync;
});
ECS_RESOURCE_DECLARE(SIAssetRoot, {
    char path[512];
});

typedef uint64_t SITextureHandle;

#define SI_INVALID_HANDLE UINT64_C(0)

typedef enum {
    SI_FILTER_NEAREST,
    SI_FILTER_LINEAR,
} SIFilterMode;

typedef enum {
    SI_BLEND_NORMAL,
    SI_BLEND_ADDITIVE,
    SI_BLEND_MULTIPLY,
} SIBlendModeValue;

ECS_RELATION_DECLARE(Layer);

extern ecs_entity_t SILayerBackground;
extern ecs_entity_t SILayerBackgroundDetail;
extern ecs_entity_t SILayerGround;
extern ecs_entity_t SILayerWorld;
extern ecs_entity_t SILayerActors;
extern ecs_entity_t SILayerEffects;
extern ecs_entity_t SILayerForeground;
extern ecs_entity_t SILayerOverlay;
extern ecs_entity_t SILayerDebug;
extern ecs_entity_t SILayerUI;

// Scene 2D
ECS_COMPONENT_DECLARE(SITransform2D, {
    float x, y;
    float rotation;
    float scale_x, scale_y;
});
ECS_COMPONENT_DECLARE(SIWorldTransform2D, {
    float x, y;
    float rotation;
    float scale_x, scale_y;
});
ECS_COMPONENT_DECLARE(SICamera2D, {
    float zoom;
    float viewport_width;
    float viewport_height;
});
ECS_COMPONENT_DECLARE(SIActiveCamera, {});
ECS_COMPONENT_DECLARE(SICameraViewport, {
    float x, y;
    float width, height;
});
ECS_COMPONENT_DECLARE(SIVirtualResolution, {
    uint32_t width, height;
    bool enabled;
    bool pixel_perfect;
});
ECS_COMPONENT_DECLARE(SIColor, { float r, g, b, a; });
ECS_COMPONENT_DECLARE(SISprite, {
    uint64_t texture;
    uint32_t frame_index;
});
ECS_COMPONENT_DECLARE(SISpriteSheet, {
    uint32_t columns, rows;
    uint32_t frame_width, frame_height;
    uint32_t margin_x, margin_y;
    uint32_t spacing_x, spacing_y;
});
ECS_COMPONENT_DECLARE(SISpriteFlip, { bool x, y; });
ECS_COMPONENT_DECLARE(SIPivot, { float x, y; });
ECS_COMPONENT_DECLARE(SIBlendMode, { uint8_t value; });
ECS_COMPONENT_DECLARE(SIAnimation, {
    uint32_t start_index;
    uint32_t end_index;
    float frame_duration;
    bool loop;
});
ECS_COMPONENT_DECLARE(SIAnimationTimer, {
    float elapsed;
    bool playing;
});

SITextureHandle siengine_load_texture(const char *path, SIFilterMode filter);
void siengine_release_texture(SITextureHandle texture);

#ifdef __cplusplus
}
#endif

#endif
