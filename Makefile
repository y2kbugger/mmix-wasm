MMIXWARE=./mmix.cs.hm.edu/mmixware/trunk
MMIXLIB=./mmix.cs.hm.edu/util/trunk/mmixlib
MMIXLIB_C_SOURCES=$(MMIXLIB)/lib*.c $(MMIXLIB)/mmix-arith.c $(MMIXLIB)/mmix-io.c

SOURCES=mmix-wasm.c

SHELL := /usr/bin/bash

EMSDK_ENV=source ./emsdk/emsdk_env.sh
EMCC=emcc

CFLAGS=-Wno-extra-tokens
EXPORTED_FUNCTIONS=-s "EXPORTED_FUNCTIONS=['_mmix_lib_initialize', '_mmix_initialize', '_mmix_boot', '_mmix_load_file', '_mmix_interact', '_mmix_fetch_instruction', '_mmix_perform_instruction', '_mmix_trace', '_mmix_dynamic_trap', '_mmix_profile', '_show_stats', '_mmix_finalize']"

all: emsdk mmix.wasm mmix.js

$(MMIXLIB)/libconfig.h: libconfig.mmix-wasm
	cp $< $@

$(MMIXLIB)/libimport.h: libimport.mmix-wasm
	cp $< $@

$(MMIXLIB_C_SOURCES): $(MMIXLIB)/libconfig.h $(MMIXLIB)/libimport.h
	$(MAKE) -C $(MMIXLIB) MMIXWARE=../../../mmixware/trunk

mmix.wasm mmix.js: $(MMIXLIB_C_SOURCES) $(SOURCES)
	$(EMSDK_ENV) && $(EMCC) $(SOURCES) $(MMIXLIB_C_SOURCES) $(CFLAGS) -I $(MMIXLIB) $(EXPORTED_FUNCTIONS) -s WASM=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'FS_createDataFile', 'FS']" -o mmix.js
	@echo run a local test server with: 'make emrun'

emrun: mmix.wasm mmix.js
	$(EMSDK_ENV) && emrun --no_browser --no_emrun_detect --port 8080 .

emsdk:
	git clone https://github.com/emscripten-core/emsdk.git
	./emsdk/emsdk install latest
	./emsdk/emsdk activate latest

clean:
	rm -f mmix.wasm
	rm -f mmix.js
	rm -f $(MMIXLIB)/abstime.h
	rm -f $(MMIXLIB)/boilerplate.w
	rm -f $(MMIXWARE)/abstime
	$(MAKE) -C $(MMIXLIB) clean
	$(MAKE) -C $(MMIXWARE) clean

clean-small:
	rm -f mmix.wasm
	rm -f mmix.js

clean-emsdk:
	rm -rf ./emsdk

