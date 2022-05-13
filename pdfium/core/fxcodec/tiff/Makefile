CC        := ~/vc/rlbox_wasm2c_sandbox/build/_deps/wasiclang-src/bin/clang
CXX       := ~/vc/rlbox_wasm2c_sandbox/build/_deps/wasiclang-src/bin/clang++
CFLAGS    := --sysroot ~/vc/rlbox_wasm2c_sandbox/build/_deps/wasiclang-src/share/wasi-sysroot/
LD        := ~/vc/rlbox_wasm2c_sandbox/build/_deps/wasiclang-src/bin/wasm-ld
LDFLAGS   := -Wl,--export-all -Wl,--no-entry -Wl,--growable-table
WASM2C    := ~/vc/rlbox_wasm2c_sandbox/build/_deps/mod_wasm2c-src/bin/wasm2c
WASM2WAT  := ~/vc/rlbox_wasm2c_sandbox/build/_deps/mod_wasm2c-src/bin/wasm2wat
RTDIR     := ~/vc/rlbox_wasm2c_sandbox/build/_deps/mod_wasm2c-src/wasm2c

my_lib.so: my_lib.c
    gcc -shared -fPIC -O3 -I $(RTDIR) -o my_lib.so my_lib.c $(RTDIR)/wasm-rt-impl.c $(RTDIR)/wasm-rt-os-unix.c $(RTDIR)/wasm-rt-os-win.c $(RTDIR)/wasm-rt-wasi.c

my_lib.c: my_lib.wasm
    $(WASM2C) my_lib.wasm -o my_lib.c

my_lib.wat: my_lib.wasm
    $(WASM2WAT) my_lib.wasm -o my_lib.wat

my_lib.wasm: lib.o wasm2c_sandbox_wrapper.o
    $(CC) $(CFLAGS) $(LDFLAGS) lib.o wasm2c_sandbox_wrapper.o -o $@

my_lib_ld.wasm: lib.o wasm2c_sandbox_wrapper.o
    $(CC) $(CFLAGS) -v -v $(LDFLAGS) lib.o wasm2c_sandbox_wrapper.o -o $@


lib.o: lib.c lib.h

wasm2c_sandbox_wrapper.o: wasm2c_sandbox_wrapper.c



# #.PHONY: app app2 tiff app_sandboxed clean

# INTEGRATION_HEADER_PATH:=include/wasm_sandbox
# RLBOX_HEADER_PATH:=include
# CXXFLAGS:=-g -Wall
# INCLUDE_FLAGS:=-I $(RLBOX_HEADER_PATH) -I $(INTEGRATION_HEADER_PATH)

# app: app.cpp lib.h lib.c
# 	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) lib.c app.cpp  -o $@

# app2: app2.cpp lib.h lib.c
# 	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) lib.c app2.cpp  -o $@

# #how to include tiffop.h? and no .c file to add
# # tiff: tiff_decoder.cpp lib.h lib.c
# # 	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) lib.c app.cpp  -o $@

# #Will not build until you sandbox the library
# app_sandboxed: app.cpp lib.h
# 	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) -pthread app.cpp -ldl -lrt -o $@

# clean:
# 	-rm -f app
# 	-rm -f app_sandboxed
