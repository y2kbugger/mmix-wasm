MMIXWARE=./mmixlib/mmixware
MMIXLIB=./mmixlib
MMIXLIB_SOURCES=./mmixlib/lib*.c ./mmixlib/mmix-arith.c ./mmixlib/mmix-io.c

SOURCES=mmix-wasm.c

EMSDK_ENV=source ./emsdk/emsdk_env.sh
EMCC=$(EMSDK_ENV) && emcc
CFLAGS=-Wno-extra-tokens

all : emsdk mmix.wasm mmix.js mmix.js

$(MMIXLIB_SOURCES) :
	cd $(MMIXLIB) && make

mmix.wasm mmix.js: $(MMIXLIB_SOURCES) $(SOURCES)
	$(EMCC) $(SOURCES) $(MMIXLIB_SOURCES) $(CFLAGS) -s WASM=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'FS_createDataFile', 'FS']" --emrun -o mmix.js
	echo run a local server like this: 'emrun --no_browser --port 8080 .'

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

clean-emsdk:
	rm -rf ./emsdk

