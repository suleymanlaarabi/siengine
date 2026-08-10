#ifndef SIENGINE_BACKEND_H
#define SIENGINE_BACKEND_H

#include "siengine.h"

/* All backend calls are main-thread-only and are made by the main-thread ECS
 * systems or the platform lifecycle. */
void sibackend_init(void);
void sibackend_shutdown(void);
void sibackend_begin_frame(void);
void sibackend_upload_instances(const void *instances, uint32_t count, uint32_t stride);
void sibackend_draw_batch(const void *batch, const void *view);
void sibackend_end_frame(void);
void sibackend_texture_create(const char *path, SIFilterMode filter, void *texture);
void sibackend_texture_destroy(void *texture);

#endif
