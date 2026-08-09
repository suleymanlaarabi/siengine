.PHONY: shaders test web web-c web-cpp

shaders:
	./tools/embed_shaders.sh

test: shaders
	bake test test

web:
	./tools/build_web.sh

web-c:
	./tools/run_web.sh c

web-cpp:
	./tools/run_web.sh cpp
