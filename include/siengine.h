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
ECS_COMPONENT_DECLARE(SIRenderOrder, {
    uint16_t layer;
    int32_t order;
});
ECS_COMPONENT_DECLARE(SIColor, { float r, g, b, a; });

#ifdef __cplusplus
}
#endif

#endif
