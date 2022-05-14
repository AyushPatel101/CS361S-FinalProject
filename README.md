## Files modified:
Main sandboxing is done in: pdfium/core/fxcodec/tiff/tiff_decoder.h, tiff_decoder.c <br>
Sandbox is created and destroyed in:pdfium/core/fxcodec/progressive_decoder.cpp<br>
Sandbox declared in: pdfium/core/fxcodec/progressive_decoder.h<br>

### Files Created:
pdfium/core/fxcodec/tiff/sandboxStructFile.h: Contains all the tainted struct declarations for the structs in the Libtiff library<br>
pdfium/core/fxcodec/tifftaintedStructs.hpp: Imports the tainted structs into one liby called mylibrar<br>