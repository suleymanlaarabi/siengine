#include "assets_internal.h"
#include "backend.h"
#include "engine_internal.h"
#include <SDL3/SDL_filesystem.h>
#include <siengine.h>
#include <string.h>

ECS_RESOURCE_DEFINE(SIAssetRoot);

static void append_asset_path(char destination[512], const char *path) {
    size_t length = strlen(destination);
    if (length && destination[length - 1] != '/')
        SDL_strlcat(destination, "/", 512);
    SDL_strlcat(destination, path, 512);
}

static const char *resolve_asset_path(const char *path, char resolved[512]) {
    if (path[0] == '/')
        return path;

    const SIAssetRoot *root = ecs_get_resource_read(SIAssetRoot);
    resolved[0] = '\0';
    if (root->path[0] != '/')
        append_asset_path(resolved, SDL_GetBasePath());
    append_asset_path(resolved, root->path);
    append_asset_path(resolved, path);
    return resolved;
}

ECS_COMPONENT_DEFINE(SITexture);
ECS_TAG_DEFINE(SITextureRelease);

SITextureHandle siengine_load_texture(const char *path, SIFilterMode filter) {
    SITextureHandle texture = ecs_new();
    SITexture request = {
        .filter = (uint8_t)filter,
        .state = SI_TEXTURE_PENDING,
    };
    size_t path_length = strlen(path);
    if (path_length >= sizeof(request.path))
        path_length = sizeof(request.path) - 1;
    memcpy(request.path, path, path_length);
    request.path[path_length] = '\0';
    ecs_set_cid(texture, ecs_id(SITexture), &request);
    return texture;
}

void siengine_release_texture(SITextureHandle texture) { ecs_add(texture, SITextureRelease); }

static void upload_textures(ecs_iter_t *it) {
    SITexture *textures = ecs_field(it, 0);
    for (uint32_t i = 0; i < it->count; i++) {
        if (textures[i].state != SI_TEXTURE_PENDING)
            continue;

        char resolved_path[512];
        const char *path = resolve_asset_path(textures[i].path, resolved_path);
        sibackend_texture_create(path, textures[i].filter, &textures[i]);
        textures[i].state = SI_TEXTURE_READY;
    }
}

static void release_textures(ecs_iter_t *it) {
    SITexture *textures = ecs_field(it, 0);
    for (uint32_t i = 0; i < it->count; i++) {
        if (textures[i].state == SI_TEXTURE_READY)
            sibackend_texture_destroy(&textures[i]);
        ecs_kill(it->entities[i]);
    }
}

void siassets_register(void) {
    ECS_COMPONENT_REGISTER(SITexture);
    ECS_COMPONENT_REGISTER(SITextureRelease);
    ECS_RESOURCE_REGISTER(SIAssetRoot);
    ecs_set_resource(SIAssetRoot, { .path = "./assets" });
    ecs_phase_t release_phase = ecs_phase({
        .name = "SiengineAssetRelease",
        .after = EcsPostRender,
    });
    ecs_system({
        .name = "SiengineUploadTextures",
        .phase = EcsPreUpdate,
        .callback = upload_textures,
        .main_thread_only = true,
        .query = {
            .components = {
                ecs_inout(SITexture),
                ecs_not(SITextureRelease),
            },
            .resources = {
                ecs_in(SIAssetRoot),
                ecs_in(SIEngineCtx),
            },
        },
    });
    ecs_system({
        .name = "SiengineReleaseTextures",
        .phase = release_phase,
        .callback = release_textures,
        .main_thread_only = true,
        .query = {
            .components = {
                ecs_inout(SITexture),
                ecs_filter(SITextureRelease),
            },
            .resources = {
                ecs_in(SIEngineCtx),
            },
        },
    });
}
