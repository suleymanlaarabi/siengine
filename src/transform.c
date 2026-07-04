#include "engine_internal.h"
#include "siecs.h"
#include "siengine.h"
#include <math.h>

ECS_MODULE_DEFINE(sitransform);

ECS_COMPONENT_DEFINE(SIPosition3d);
ECS_COMPONENT_DEFINE(SIScale3d);
ECS_COMPONENT_DEFINE(SIRotation3d);
ECS_COMPONENT_DEFINE(SICamera3d);
ECS_COMPONENT_DEFINE(SIActiveCamera);
ECS_COMPONENT_DEFINE(SICube);
ECS_COMPONENT_DEFINE(SIColor);

static SIMat4 si_mat4_identity(void) {
    return (SIMat4){ .m = {
                         1.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         1.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         1.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         1.0f,
                     } };
}

SIMat4 si_mat4_mul(SIMat4 lhs, SIMat4 rhs) {
    SIMat4 out = { 0 };

    for (uint32_t col = 0; col < 4; col++) {
        for (uint32_t row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (uint32_t k = 0; k < 4; k++) {
                sum += lhs.m[k * 4 + row] * rhs.m[col * 4 + k];
            }
            out.m[col * 4 + row] = sum;
        }
    }

    return out;
}

static SIMat4 si_mat4_translation(SIPosition3d position) {
    SIMat4 out = si_mat4_identity();
    out.m[12] = position.x;
    out.m[13] = position.y;
    out.m[14] = position.z;
    return out;
}

static SIMat4 si_mat4_scale(SIScale3d scale) {
    SIMat4 out = si_mat4_identity();
    out.m[0] = scale.x;
    out.m[5] = scale.y;
    out.m[10] = scale.z;
    return out;
}

static SIMat4 si_mat4_rotation_x(float angle) {
    SIMat4 out = si_mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);

    out.m[5] = c;
    out.m[6] = s;
    out.m[9] = -s;
    out.m[10] = c;

    return out;
}

static SIMat4 si_mat4_rotation_y(float angle) {
    SIMat4 out = si_mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);

    out.m[0] = c;
    out.m[2] = -s;
    out.m[8] = s;
    out.m[10] = c;

    return out;
}

static SIMat4 si_mat4_rotation_z(float angle) {
    SIMat4 out = si_mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);

    out.m[0] = c;
    out.m[1] = s;
    out.m[4] = -s;
    out.m[5] = c;

    return out;
}

static SIMat4 si_mat4_rotation(SIRotation3d rotation) {
    SIMat4 rx = si_mat4_rotation_x(rotation.x);
    SIMat4 ry = si_mat4_rotation_y(rotation.y);
    SIMat4 rz = si_mat4_rotation_z(rotation.z);

    return si_mat4_mul(rz, si_mat4_mul(ry, rx));
}

SIMat4 si_mat4_model(SIPosition3d position, SIRotation3d rotation, SIScale3d scale) {
    return si_mat4_mul(
        si_mat4_translation(position),
        si_mat4_mul(si_mat4_rotation(rotation), si_mat4_scale(scale))
    );
}

SIMat4 si_mat4_view(SIPosition3d position, SIRotation3d rotation) {
    SIPosition3d inverse_position = { -position.x, -position.y, -position.z };
    SIRotation3d inverse_rotation = { -rotation.x, -rotation.y, -rotation.z };

    return si_mat4_mul(si_mat4_rotation(inverse_rotation), si_mat4_translation(inverse_position));
}

SIMat4 si_mat4_perspective(float fov_y, float aspect, float near_clip, float far_clip) {
    float f = 1.0f / tanf(fov_y * 0.5f);
    float range = far_clip - near_clip;

    return (SIMat4){ .m = {
                         f / aspect,
                         0.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         f,
                         0.0f,
                         0.0f,
                         0.0f,
                         0.0f,
                         far_clip / range,
                         1.0f,
                         0.0f,
                         0.0f,
                         -(near_clip * far_clip) / range,
                         0.0f,
                     } };
}

void sitransform_import(ecs_world_t *world, const sitransform_props_t *props) {
    (void)props;

    ECS_COMPONENT_REGISTER(world, SIPosition3d);
    ECS_COMPONENT_REGISTER(world, SIScale3d);
    ECS_COMPONENT_REGISTER(world, SIRotation3d);
    ECS_COMPONENT_REGISTER(world, SICamera3d);
    ECS_COMPONENT_REGISTER(world, SIActiveCamera);
    ECS_COMPONENT_REGISTER(world, SICube);
    ECS_COMPONENT_REGISTER(world, SIColor);
}
