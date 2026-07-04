#include <math.h>
#include <test.h>

static void test_float_near(float expected, float actual) {
    float delta = fabsf(expected - actual);
    test_assert(delta < 0.0001f);
}

void math_model_applies_translation_scale(void) {
    SIMat4 model = si_mat4_model(
        (SIPosition3d){ .x = 1.0f, .y = 2.0f, .z = 3.0f },
        (SIRotation3d){ .x = 0.0f, .y = 0.0f, .z = 0.0f },
        (SIScale3d){ .x = 2.0f, .y = 3.0f, .z = 4.0f }
    );

    test_float_near(2.0f, model.m[0]);
    test_float_near(3.0f, model.m[5]);
    test_float_near(4.0f, model.m[10]);
    test_float_near(1.0f, model.m[12]);
    test_float_near(2.0f, model.m[13]);
    test_float_near(3.0f, model.m[14]);
    test_float_near(1.0f, model.m[15]);
}

void math_perspective_has_expected_terms(void) {
    SIMat4 projection = si_mat4_perspective(1.5707963f, 2.0f, 0.1f, 100.0f);

    test_float_near(0.5f, projection.m[0]);
    test_float_near(1.0f, projection.m[5]);
    test_float_near(100.0f / 99.9f, projection.m[10]);
    test_float_near(1.0f, projection.m[11]);
    test_float_near(-(0.1f * 100.0f) / 99.9f, projection.m[14]);
    test_float_near(0.0f, projection.m[15]);
}
