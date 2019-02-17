all:
	emcc mmix-wasm.c ./src/mmixlib/lib*.c ./src/mmixlib/mmix-arith.c ./src/mmixlib/mmix-io.c -s WASM=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'FS_createDataFile', 'FS']" -o mmix.wasm  && emrun --no_browser --port 8080 .
