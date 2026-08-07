.PHONY: shaders test

shaders:
	./tools/embed_shaders.sh

test: shaders
	bake test test
