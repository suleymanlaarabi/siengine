#include "render_internal.h"
#include "siecs.h"
#include <math.h>
static SIRenderBatch *begin_batch(SIRenderState *render) {
    SIRenderBatch *batch = sicore_vec_push_empty(&render->batches, sizeof(*batch));
    *batch = (SIRenderBatch){
        .instance_offset = render->instances.size,
    };
    return batch;
}

static void finish_batch(SIRenderState *render, SIRenderBatch *batch) {
    uint32_t count = render->instances.size - batch->instance_offset;
    if (!count) {
        sicore_vec_remove_last(&render->batches);
        return;
    }
    batch->instance_count = count;
}

static bool instance_visible(
    const SIRenderState *render,
    const SIWorldTransform2D *transform,
    float width,
    float height
) {
    float scaled_width = fabsf(transform->scale_x * width);
    float scaled_height = fabsf(transform->scale_y * height);
    float radius = sqrtf(scaled_width * scaled_width + scaled_height * scaled_height);
    const SIRenderView *views = sicore_vec_data(&render->views, SIRenderView);
    for (uint32_t i = 0; i < render->views.size; i++) {
        const SIRenderView *view = &views[i];
        if (transform->x + radius >= view->left && transform->x - radius <= view->right &&
            transform->y + radius >= view->top && transform->y - radius <= view->bottom)
            return true;
    }
    return false;
}

static void extract_transform(
    SIInstance2D *instance,
    const SIWorldTransform2D *transform,
    const SIColor *color
) {
    instance->x = transform->x;
    instance->y = transform->y;
    instance->rotation = transform->rotation;
    instance->scale_x = transform->scale_x;
    instance->scale_y = transform->scale_y;
    instance->color = *color;
}

static void extract_sprite_batch(
    SIRenderState *render,
    SIRenderBatch *batch,
    const SIWorldTransform2D *transforms,
    const SIColor *colors,
    const SISprite *sprites,
    const SISpriteFlip *flips,
    const SIMaterial2D *material,
    const SISpriteSheet *sheet,
    const SIPivot *pivot,
    const SIBlendMode *blend,
    const SITexture *texture,
    uint32_t count
) {
    float width = sheet->columns ? (float)sheet->frame_width : (float)texture->width;
    float height = sheet->columns ? (float)sheet->frame_height : (float)texture->height;

    batch->pipeline = SI_PIPELINE_SPRITE;
    batch->geometry = SI_GEOMETRY_QUAD;
    batch->texture = material->texture;
    batch->gpu_texture = texture->gpu_handle;
    batch->texture_width = texture->width;
    batch->texture_height = texture->height;
    batch->filter = material->filter;
    batch->blend = blend->value;
    batch->pivot_x = pivot->x;
    batch->pivot_y = pivot->y;
    batch->sheet = *sheet;
    batch->has_sheet = sheet->columns != 0;

    for (uint32_t i = 0; i < count; i++) {
        if (!instance_visible(render, &transforms[i], width, height))
            continue;
        SIInstance2D *instance = sicore_vec_push_empty(&render->instances, sizeof(*instance));
        extract_transform(instance, &transforms[i], &colors[i]);
        instance->width = width;
        instance->height = height;
        instance->frame_index = (float)sprites[i].frame_index;
        instance->flip_x = flips ? (float)flips[i].x : 0.0f;
        instance->flip_y = flips ? (float)flips[i].y : 0.0f;
    }
    finish_batch(render, batch);
}

static void extract_circle_batch(
    SIRenderState *render,
    SIRenderBatch *batch,
    const SIWorldTransform2D *transforms,
    const SIColor *colors,
    const SICircle *circles,
    const SIMaterial2D *material,
    const SIPivot *pivot,
    const SIBlendMode *blend,
    uint32_t count
) {
    batch->pipeline = SI_PIPELINE_CIRCLE;
    batch->geometry = SI_GEOMETRY_QUAD;
    batch->filter = material->filter;
    batch->blend = blend->value;
    batch->pivot_x = pivot->x;
    batch->pivot_y = pivot->y;

    for (uint32_t i = 0; i < count; i++) {
        float size = circles[i].radius * 2.0f;
        if (!instance_visible(render, &transforms[i], size, size))
            continue;
        SIInstance2D *instance = sicore_vec_push_empty(&render->instances, sizeof(*instance));
        extract_transform(instance, &transforms[i], &colors[i]);
        instance->width = size;
        instance->height = size;
    }
    finish_batch(render, batch);
}

static void extract_rectangle_batch(
    SIRenderState *render,
    SIRenderBatch *batch,
    const SIWorldTransform2D *transforms,
    const SIColor *colors,
    const SIRectangle *rectangles,
    const SIMaterial2D *material,
    const SIPivot *pivot,
    const SIBlendMode *blend,
    uint32_t count
) {
    batch->pipeline = SI_PIPELINE_SHAPE;
    batch->geometry = SI_GEOMETRY_QUAD;
    batch->filter = material->filter;
    batch->blend = blend->value;
    batch->pivot_x = pivot->x;
    batch->pivot_y = pivot->y;

    for (uint32_t i = 0; i < count; i++) {
        if (!instance_visible(render, &transforms[i], rectangles[i].width, rectangles[i].height))
            continue;
        SIInstance2D *instance = sicore_vec_push_empty(&render->instances, sizeof(*instance));
        extract_transform(instance, &transforms[i], &colors[i]);
        instance->width = rectangles[i].width;
        instance->height = rectangles[i].height;
    }
    finish_batch(render, batch);
}

static void extract_triangle_batch(
    SIRenderState *render,
    SIRenderBatch *batch,
    const SIWorldTransform2D *transforms,
    const SIColor *colors,
    const SITriangle *triangles,
    const SIMaterial2D *material,
    const SIPivot *pivot,
    const SIBlendMode *blend,
    uint32_t count
) {
    batch->pipeline = SI_PIPELINE_SHAPE;
    batch->geometry = SI_GEOMETRY_TRIANGLE;
    batch->filter = material->filter;
    batch->blend = blend->value;
    batch->pivot_x = pivot->x;
    batch->pivot_y = pivot->y;

    for (uint32_t i = 0; i < count; i++) {
        if (!instance_visible(render, &transforms[i], triangles[i].base, triangles[i].height))
            continue;
        SIInstance2D *instance = sicore_vec_push_empty(&render->instances, sizeof(*instance));
        extract_transform(instance, &transforms[i], &colors[i]);
        instance->width = triangles[i].base;
        instance->height = triangles[i].height;
    }
    finish_batch(render, batch);
}

void sirender_begin_frame(ecs_iter_t *it) {
    (void)it;
    sibackend_begin_frame();
    SIRenderState *render = ecs_get_resource(SIRenderState);
    sicore_vec_clear(&render->views);
    sicore_vec_clear(&render->batches);
    sicore_vec_clear(&render->instances);
}

void sirender_extract_cameras(ecs_iter_t *it) {
    SIRenderState *render = ecs_get_resource(SIRenderState);
    const SICamera2D *cameras = ecs_field(it, 0);
    const SIWorldTransform2D *transforms = ecs_field(it, 1);
    const SICameraViewport *viewports = ecs_field(it, 2);
    const SIVirtualResolution *virtual_resolution = ecs_field(it, 3);

    for (uint32_t i = 0; i < it->count; i++) {
        SIRenderView *view = sicore_vec_push_empty(&render->views, sizeof(*view));
        float width = cameras[i].viewport_width / cameras[i].zoom;
        float height = cameras[i].viewport_height / cameras[i].zoom;

        *view = (SIRenderView){
            .left = transforms[i].x - width * 0.5f,
            .top = transforms[i].y - height * 0.5f,
            .right = transforms[i].x + width * 0.5f,
            .bottom = transforms[i].y + height * 0.5f,
            .viewport_x = viewports[i].x,
            .viewport_y = viewports[i].y,
            .viewport_width = viewports[i].width,
            .viewport_height = viewports[i].height,
            .virtual_width = virtual_resolution ? virtual_resolution[i].width : 0,
            .virtual_height = virtual_resolution ? virtual_resolution[i].height : 0,
            .virtual_enabled = virtual_resolution != NULL,
            .pixel_perfect = virtual_resolution ? virtual_resolution[i].pixel_perfect : false,
        };
    }
}

void sirender_extract_renderables(ecs_iter_t *it) {
    SIRenderState *render = ecs_get_resource(SIRenderState);
    const SIWorldTransform2D *transforms = ecs_field(it, 0);
    const SIColor *colors = ecs_field(it, 1);
    const SISprite *sprites = ecs_field(it, 2);
    const SISpriteFlip *flips = ecs_field(it, 3);
    const SICircle *circles = ecs_field(it, 4);
    const SIRectangle *rectangles = ecs_field(it, 5);
    const SITriangle *triangles = ecs_field(it, 6);
    const SIMaterial2D *material = ecs_field(it, 7);
    const SISpriteSheet *sheet = ecs_field(it, 8);
    const SIPivot *pivot = ecs_field(it, 9);
    const SIBlendMode *blend = ecs_field(it, 10);
    const ecs_entity_t layer = ecs_target_shared(it, Layer);

    SIRenderBatch *batch = begin_batch(render);
    batch->layer = layer;

    if (sprites) {
        const SITexture *texture = ecs_get(material->texture, SITexture);
        if (texture->state != SI_TEXTURE_READY)
            return;

        extract_sprite_batch(
            render,
            batch,
            transforms,
            colors,
            sprites,
            flips,
            material,
            sheet,
            pivot,
            blend,
            texture,
            it->count
        );
    } else if (circles) {
        extract_circle_batch(
            render,
            batch,
            transforms,
            colors,
            circles,
            material,
            pivot,
            blend,
            it->count
        );
    } else if (rectangles) {
        extract_rectangle_batch(
            render,
            batch,
            transforms,
            colors,
            rectangles,
            material,
            pivot,
            blend,
            it->count
        );
    } else {
        extract_triangle_batch(
            render,
            batch,
            transforms,
            colors,
            triangles,
            material,
            pivot,
            blend,
            it->count
        );
    }
}

void sirender_draw_window(ecs_iter_t *it) {
    (void)it;
    SIRenderState *render = ecs_get_resource(SIRenderState);
    SIRenderView *views = sicore_vec_data(&render->views, SIRenderView);
    SIRenderBatch *batches = sicore_vec_data(&render->batches, SIRenderBatch);
    sibackend_upload_instances(
        render->instances.data,
        render->instances.size,
        sizeof(SIInstance2D)
    );
    for (uint32_t view_index = 0; view_index < render->views.size; view_index++) {
        for (uint32_t batch_index = 0; batch_index < render->batches.size; batch_index++)
            sibackend_draw_batch(&batches[batch_index], &views[view_index]);
    }
}

void sirender_end_frame(ecs_iter_t *it) {
    (void)it;
    sibackend_end_frame();
}
