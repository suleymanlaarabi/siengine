#include "assets_internal.h"
#include "backend.h"
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

static void texture_on_remove(ecs_entity_t entity, ecs_component_t component, void *value) {
    (void)entity;
    (void)component;
    sibackend_texture_destroy(value);
}

ECS_COMPONENT_DEFINE(SITexture, .on_remove = texture_on_remove);

SITextureHandle siengine_load_texture(const char *path, SIFilterMode filter) {
    char resolved_path[512];
    path = resolve_asset_path(path, resolved_path);

    SITextureHandle texture = ecs_new();
    ecs_add(texture, SITexture);
    sibackend_texture_create(path, filter, ecs_get(texture, SITexture));
    return texture;
}

void siengine_release_texture(SITextureHandle texture) { ecs_kill(texture); }

void siassets_register(void) {
    ECS_COMPONENT_REGISTER(SITexture);
    ECS_RESOURCE_REGISTER(SIAssetRoot);
    ecs_set_resource(SIAssetRoot, { .path = "./assets" });
}
