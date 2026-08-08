#!/bin/sh
set -eu

output="src/render_shaders.c"
temporary="$(mktemp)"
trap 'rm -f "$temporary"' EXIT

{
    printf '%s\n\n' '#include "render_shaders.h"'
    xxd -i shaders/sprite.vert.spv |
        sed \
            -e 's/^unsigned char shaders_sprite_vert_spv\[\]/const unsigned char si_sprite_vertex_shader[]/' \
            -e 's/^unsigned int shaders_sprite_vert_spv_len/const unsigned int si_sprite_vertex_shader_size/'
    printf '\n'
    xxd -i shaders/sprite.frag.spv |
        sed \
            -e 's/^unsigned char shaders_sprite_frag_spv\[\]/const unsigned char si_sprite_fragment_shader[]/' \
            -e 's/^unsigned int shaders_sprite_frag_spv_len/const unsigned int si_sprite_fragment_shader_size/'
    printf '\n'
    xxd -i shaders/shape.frag.spv |
        sed \
            -e 's/^unsigned char shaders_shape_frag_spv\[\]/const unsigned char si_shape_fragment_shader[]/' \
            -e 's/^unsigned int shaders_shape_frag_spv_len/const unsigned int si_shape_fragment_shader_size/'
    printf '\n'
    xxd -i shaders/circle.frag.spv |
        sed \
            -e 's/^unsigned char shaders_circle_frag_spv\[\]/const unsigned char si_circle_fragment_shader[]/' \
            -e 's/^unsigned int shaders_circle_frag_spv_len/const unsigned int si_circle_fragment_shader_size/'
} > "$temporary"

if ! cmp -s "$temporary" "$output"; then
    mv "$temporary" "$output"
    trap - EXIT
fi
