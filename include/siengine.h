#ifndef SIENGINE_H
#define SIENGINE_H

/* This generated file contains includes for project dependencies */
#include "siecs.h"
#include "siengine/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

ECS_MODULE_DECLARE(siengine, {});
ECS_COMPONENT_DECLARE(SIWindow, {
    char *title;
    uint32_t width;
    uint32_t height;
    bool resizable;
    bool vsync;
});

// Transform
ECS_COMPONENT_DECLARE(SIPosition3d, { float x, y, z; });
ECS_COMPONENT_DECLARE(SIRotation3d, { float x, y, z; }); // radians
ECS_COMPONENT_DECLARE(SIScale3d, { float x, y, z; });

// Rendering
ECS_COMPONENT_DECLARE(SICube, {});
ECS_COMPONENT_DECLARE(SIColor, { float r, g, b, a; });

// Camera
ECS_COMPONENT_DECLARE(SICamera3d, {
    float fov_y;
    float near_clip;
    float far_clip;
});
ECS_COMPONENT_DECLARE(SIActiveCamera, {});

typedef struct {
    float m[16];
} SIMat4;

SIMat4 si_mat4_model(SIPosition3d position, SIRotation3d rotation, SIScale3d scale);
SIMat4 si_mat4_perspective(float fov_y, float aspect, float near_clip, float far_clip);
SIMat4 si_mat4_view(SIPosition3d position, SIRotation3d rotation);
SIMat4 si_mat4_mul(SIMat4 lhs, SIMat4 rhs);

#ifdef __cplusplus
}
#endif

#endif
