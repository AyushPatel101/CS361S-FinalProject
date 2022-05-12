.PHONY: app app2 tiff app_sandboxed clean

INTEGRATION_HEADER_PATH:=include/wasm_sandbox
RLBOX_HEADER_PATH:=include
CXXFLAGS:=-g -Wall
INCLUDE_FLAGS:=-I $(RLBOX_HEADER_PATH) -I $(INTEGRATION_HEADER_PATH)

app: app.cpp lib.h lib.c
	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) lib.c app.cpp  -o $@

app2: app2.cpp lib.h lib.c
	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) lib.c app2.cpp  -o $@

#how to include tiffop.h? and no .c file to add
# tiff: tiff_decoder.cpp lib.h lib.c
# 	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) lib.c app.cpp  -o $@

#Will not build until you sandbox the library
app_sandboxed: app.cpp lib.h
	$(CXX) -std=c++17 $(CXXFLAGS) $(INCLUDE_FLAGS) -pthread app.cpp -ldl -lrt -o $@

clean:
	-rm -f app
	-rm -f app_sandboxed
