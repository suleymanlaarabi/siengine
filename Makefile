.PHONY: shaders test

shaders:
	glslc -fshader-stage=vertex shaders/vertex.glsl -o shaders/cube.vert.spv
	glslc -fshader-stage=fragment shaders/fragment.glsl -o shaders/cube.frag.spv

test: shaders
	bake test test
