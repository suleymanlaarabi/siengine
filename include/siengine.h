#ifndef SIENGINE_H
#define SIENGINE_H

/* This generated file contains includes for project dependencies */
#include "siecs.h"
#include "siengine/bake_config.h"

#ifdef __cplusplus
#include <cstring>
extern "C" {
#endif

#define DEG2RAD(deg) ((deg) * 0.01745329251994329576923690768489)

ECS_MODULE_DECLARE(siengine, {});

ECS_RESOURCE_DECLARE_CPP(
    SIWindow,
    ECS_CPP_FIELDS(char title[128]; uint32_t width; uint32_t height; bool resizable; bool vsync;),
    ECS_CPP_METHODS(
        SIWindow() : title{"Siengine"}, width(1280), height(720), resizable(true), vsync(true) {}
        SIWindow(const char *window_title) : SIWindow() { std::strcpy(title, window_title); }
    )
);

ECS_RESOURCE_DECLARE_CPP(
    SIAssetRoot,
    ECS_CPP_FIELDS(const char *path;),
    ECS_CPP_METHODS(
        SIAssetRoot() : path{"./assets"} {}
        SIAssetRoot(const char *path) : path{path} {}
    )
);

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

ECS_COMPONENT_DECLARE_CPP(
    SITransform2D,
    ECS_CPP_FIELDS(float x; float y; float rotation; float scale_x; float scale_y;),
    ECS_CPP_METHODS(
        SITransform2D() : x(0), y(0), rotation(0), scale_x(1), scale_y(1) {}
        static SITransform2D from_xy(float x, float y) {
            SITransform2D transform;
            transform.x = x;
            transform.y = y;
            return transform;
        }
        SITransform2D with_scale(float value) {
            this->scale_x = value;
            this->scale_y = value;
            return *this;
        }
    )
);

ECS_COMPONENT_DECLARE_CPP(
    SIWorldTransform2D,
    ECS_CPP_FIELDS(float x; float y; float rotation; float scale_x; float scale_y;),
    ECS_CPP_METHODS(SIWorldTransform2D() : x(0), y(0), rotation(0), scale_x(1), scale_y(1) {})
);
ECS_COMPONENT_DECLARE_CPP(
    SICamera2D,
    ECS_CPP_FIELDS(float zoom; float viewport_width; float viewport_height;),
    ECS_CPP_METHODS(SICamera2D() : zoom(1), viewport_width(320), viewport_height(180) {})
);
ECS_COMPONENT_DECLARE_CPP(
    SICameraViewport,
    ECS_CPP_FIELDS(float x; float y; float width; float height;),
    ECS_CPP_METHODS(SICameraViewport() : width(1), height(1) {})
);
ECS_COMPONENT_DECLARE(SIVirtualResolution, {
    uint32_t width;
    uint32_t height;
    bool pixel_perfect;
});
ECS_COMPONENT_DECLARE_CPP(
    SIColor,
    ECS_CPP_FIELDS(float r; float g; float b; float a;),
    ECS_CPP_METHODS(SIColor() : r(1), g(1), b(1), a(1) {})
);
ECS_COMPONENT_DECLARE_CPP(
    SISprite,
    ECS_CPP_FIELDS(uint64_t texture; uint32_t frame_index;),
    ECS_CPP_METHODS(
        SISprite() : texture(SI_INVALID_HANDLE), frame_index(0) {}
        SISprite(uint64_t sprite_texture) : texture(sprite_texture), frame_index(0) {}
    )
);
ECS_COMPONENT_DECLARE_CPP(
    SICircle,
    ECS_CPP_FIELDS(float radius;),
    ECS_CPP_METHODS(SICircle() : radius(1) {} SICircle(float circle_radius) : radius(circle_radius) {})
);
ECS_COMPONENT_DECLARE_CPP(
    SIRectangle,
    ECS_CPP_FIELDS(float width; float height;),
    ECS_CPP_METHODS(
        SIRectangle() : width(1), height(1) {}
        SIRectangle(float rectangle_width, float rectangle_height)
            : width(rectangle_width), height(rectangle_height) {}
    )
);
ECS_COMPONENT_DECLARE_CPP(
    SITriangle,
    ECS_CPP_FIELDS(float base; float height;),
    ECS_CPP_METHODS(
        SITriangle() : base(1), height(1) {}
        SITriangle(float triangle_base, float triangle_height)
            : base(triangle_base), height(triangle_height) {}
    )
);
ECS_COMPONENT_DECLARE(SISpriteSheet, {
    uint32_t columns, rows;
    uint32_t frame_width, frame_height;
    uint32_t margin_x, margin_y;
    uint32_t spacing_x, spacing_y;
});
ECS_COMPONENT_DECLARE_CPP(
    SISpriteFlip,
    ECS_CPP_FIELDS(bool x; bool y;),
    ECS_CPP_METHODS(SISpriteFlip() : x(false), y(false) {})
);
ECS_COMPONENT_DECLARE_CPP(
    SIPivot,
    ECS_CPP_FIELDS(float x; float y;),
    ECS_CPP_METHODS(SIPivot() : x(0.5f), y(0.5f) {})
);

ECS_COMPONENT_DECLARE_CPP(
    SIBlendMode,
    ECS_CPP_FIELDS(uint8_t value;),
    ECS_CPP_METHODS(SIBlendMode() : value(SI_BLEND_NORMAL) {})
);
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
