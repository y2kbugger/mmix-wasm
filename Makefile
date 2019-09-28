MMIXWARE=./mmixlib/mmixware
MMIXLIB=./mmixlib
MMIXLIB_SOURCES=./mmixlib/lib*.c ./mmixlib/mmix-arith.c ./mmixlib/mmix-io.c

SOURCES=mmix-wasm.c

SHELL := /usr/bin/bash

EMSDK_ENV=source ./emsdk/emsdk_env.sh
EMCC=emcc

CFLAGS=-Wno-extra-tokens
EXPORTED_FUNCTIONS=-s "EXPORTED_FUNCTIONS=['_mmix_lib_initialize', '_mmix_initialize', '_mmix_boot', '_mmix_load_file', '_mmix_interact', '_mmix_fetch_instruction', '_mmix_perform_instruction', '_mmix_trace', '_mmix_dynamic_trap', '_mmix_profile', '_show_stats', '_mmix_finalize']"

all : emsdk mmix.wasm mmix.js mmix.js

$(MMIXLIB_SOURCES) :
	cd $(MMIXLIB) && make

mmix.wasm mmix.js : $(MMIXLIB_SOURCES) $(SOURCES)
	$(EMSDK_ENV) && $(EMCC) $(SOURCES) $(MMIXLIB_SOURCES) $(CFLAGS) $(EXPORTED_FUNCTIONS) -s WASM=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'FS_createDataFile', 'FS']" -o mmix.js
	echo run a local server like this: 'emrun --no_browser --port 8080 .'

emrun : mmix.wasm mmix.js
	$(EMSDK_ENV) && emrun --no_browser --no_emrun_detect --port 8080 .

emsdk :
	git clone https://github.com/emscripten-core/emsdk.git
	./emsdk/emsdk install latest
	./emsdk/emsdk activate latest

clean:
	rm -f mmix.wasm
	rm -f mmix.js
	rm -f $(MMIXLIB)/abstime.h
	rm -f $(MMIXLIB)/boilerplate.w
	rm -f $(MMIXWARE)/abstime
	cd $(MMIXLIB) && make clean
	cd $(MMIXWARE) && make clean

clean-small:
	rm -f mmix.wasm
	rm -f mmix.js

clean-emsdk:
	rm -rf ./emsdk

