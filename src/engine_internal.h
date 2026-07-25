#ifndef SIENGINE_INTERNAL_H
#define SIENGINE_INTERNAL_H

#include "siecs.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

ECS_RESOURCE_DECLARE(SIEngineCtx, { SDL_GPUDevice *primary_gpu; });

ECS_COMPONENT_DECLARE(SIWindowHandle, { ptr handle; });

ECS_MODULE_DECLARE(sitransform, {});

void siwindow_register();
void sirender_register();
void sirender_shutdown();

#endif
