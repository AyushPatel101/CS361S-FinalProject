// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/tiff/tiff_decoder.h"

#include <limits>
#include <memory>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "third_party/base/check.h"
#include "third_party/base/notreached.h"
#include "third_party/base/numerics/safe_conversions.h"

#include "taintedStructs.hpp"
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
using namespace std;
extern "C" {
#include "third_party/libtiff/tiffiop.h"
}  // extern C

#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USEstatic_CALLS() rlbox_noop_sandbox_lookup_symbol
#include "rlbox_noop_sandbox.hpp"
#include "rlbox.hpp"
using namespace rlbox;

namespace {

// For use with std::unique_ptr<TIFF>.
struct TiffDeleter {
  inline void operator()(TIFF* context) { TIFFClose(context); }
};

}  // namespace

class CTiffContext final : public ProgressiveDecoderIface::Context {
 public:
  CTiffContext() = default;
  ~CTiffContext() override = default;

  bool InitDecoder(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<IFX_SeekableReadStream>& file_ptr);
  bool LoadFrameInfo(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, int32_t frame,
                     int32_t* width,
                     int32_t* height,
                     int32_t* comps,
                     int32_t* bpc,
                     CFX_DIBAttribute* pAttribute);
  bool Decode(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap);

  RetainPtr<IFX_SeekableReadStream> io_in() const { return m_io_in; }
  uint32_t offset() const { return m_offset; }
  void set_offset(uint32_t offset) { m_offset = offset; }

 private:
  bool IsSupport(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap) const;
  void SetPalette(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap, uint16_t bps);
  bool Decode1bppRGB(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                     int32_t height,
                     int32_t width,
                     uint16_t bps,
                     uint16_t spp);
  bool Decode8bppRGB(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox,const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                     int32_t height,
                     int32_t width,
                     uint16_t bps,
                     uint16_t spp);
  bool Decode24bppRGB(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox,const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                      int32_t height,
                      int32_t width,
                      uint16_t bps,
                      uint16_t spp);

  RetainPtr<IFX_SeekableReadStream> m_io_in;
  uint32_t m_offset = 0;
  std::unique_ptr<TIFF, TiffDeleter> m_tif_ctx;
};

void* _TIFFcalloc(tmsize_t nmemb, tmsize_t siz) {
  return FXMEM_DefaultCalloc(nmemb, siz);
}

void* _TIFFmalloc(tmsize_t size) {
  return FXMEM_DefaultAlloc(size);
}

void _TIFFfree(void* ptr) {
  if (ptr)
    FXMEM_DefaultFree(ptr);
}

void* _TIFFrealloc(void* ptr, tmsize_t size) {
  return FXMEM_DefaultRealloc(ptr, size);
}

void _TIFFmemset(void* ptr, int val, tmsize_t size) {
  memset(ptr, val, static_cast<size_t>(size));
}

void _TIFFmemcpy(void* des, const void* src, tmsize_t size) {
  memcpy(des, src, static_cast<size_t>(size));
}

int _TIFFmemcmp(const void* ptr1, const void* ptr2, tmsize_t size) {
  return memcmp(ptr1, ptr2, static_cast<size_t>(size));
}

TIFFErrorHandler _TIFFwarningHandler = nullptr;
TIFFErrorHandler _TIFFerrorHandler = nullptr;

namespace {

tsize_t tiff_read(thandle_t context, tdata_t buf, tsize_t length) {
  CTiffContext* pTiffContext = reinterpret_cast<CTiffContext*>(context);
  FX_SAFE_UINT32 increment = pTiffContext->offset();
  increment += length;
  if (!increment.IsValid())
    return 0;

  FX_FILESIZE offset = pTiffContext->offset();
  if (!pTiffContext->io_in()->ReadBlockAtOffset(buf, offset, length))
    return 0;

  pTiffContext->set_offset(increment.ValueOrDie());
  if (offset + length > pTiffContext->io_in()->GetSize()) {
    return pdfium::base::checked_cast<tsize_t>(
        pTiffContext->io_in()->GetSize() - offset);
  }
  return length;
}

tsize_t tiff_write(thandle_t context, tdata_t buf, tsize_t length) {
  NOTREACHED();
  return 0;
}

toff_t tiff_seek(thandle_t context, toff_t offset, int whence) {
  CTiffContext* pTiffContext = reinterpret_cast<CTiffContext*>(context);
  FX_SAFE_FILESIZE safe_offset = offset;
  if (!safe_offset.IsValid())
    return static_cast<toff_t>(-1);
  FX_FILESIZE file_offset = safe_offset.ValueOrDie();

  switch (whence) {
    case 0: {
      if (file_offset > pTiffContext->io_in()->GetSize())
        return static_cast<toff_t>(-1);
      pTiffContext->set_offset(
          pdfium::base::checked_cast<uint32_t>(file_offset));
      return pTiffContext->offset();
    }
    case 1: {
      FX_SAFE_UINT32 new_increment = pTiffContext->offset();
      new_increment += file_offset;
      if (!new_increment.IsValid())
        return static_cast<toff_t>(-1);
      pTiffContext->set_offset(new_increment.ValueOrDie());
      return pTiffContext->offset();
    }
    case 2: {
      if (pTiffContext->io_in()->GetSize() < file_offset)
        return static_cast<toff_t>(-1);
      pTiffContext->set_offset(pdfium::base::checked_cast<uint32_t>(
          pTiffContext->io_in()->GetSize() - file_offset));
      return pTiffContext->offset();
    }
    default:
      return static_cast<toff_t>(-1);
  }
}

int tiff_close(thandle_t context) {
  return 0;
}

toff_t tiff_get_size(thandle_t context) {
  CTiffContext* pTiffContext = reinterpret_cast<CTiffContext*>(context);
  return static_cast<toff_t>(pTiffContext->io_in()->GetSize());
}

int tiff_map(thandle_t context, tdata_t*, toff_t*) {
  return 0;
}

void tiff_unmap(thandle_t context, tdata_t, toff_t) {}

TIFF* tiff_open(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, void* context, const char* mode) {

  //invoke sandbox
  const char* string = "Tiff Image";
  auto size_1= strlen(string) + 1;
  auto string_T = sandbox.malloc_in_sandbox<char>(size_1);
  std::strncpy(string_T.unverified_safe_pointer_because(size_1, "writing to memory"), string, size_1);

  auto size_2 = strlen(mode) + 1;
  auto mode_T = sandbox.malloc_in_sandbox<char>(size_2);
  std::strncpy(mode_T.unverified_safe_pointer_because(size_2, "writing to memory"), mode, size_2);



  auto tiff_read_T = sandbox.get_sandbox_function_address(tiff_read);
  auto tiff_write_T = sandbox.get_sandbox_function_address(tiff_write);
  auto tiff_seek_T = sandbox.get_sandbox_function_address(tiff_seek);
  auto tiff_close_T = sandbox.get_sandbox_function_address(tiff_close);
  auto tiff_get_size_T = sandbox.get_sandbox_function_address(tiff_get_size);
  auto tiff_map_T = sandbox.get_sandbox_function_address(tiff_map);

 auto tiff_unmap_T = sandbox.get_sandbox_function_address(tiff_unmap);
 TIFF* tif = sandbox.invoke_sandbox_function(TIFFClientOpen, string_T, mode_T, context_T, tiff_read_T, tiff_write_T, tiff_seek_T, tiff_close_T, tiff_get_size_T, tiff_map_T, tiff_unmap_T).copy_and_verify([](TIFF* tiff) {
   assert(tiff!=nullptr)
   return tiff;
 });

  auto context_temp= sandbox.malloc_in_sandbox<void>(sizeof(void*));
  rlbox::memcpy(sandbox, context_temp, context, sizeof(void*));
  auto context_T = rlbox::sandbox_reinterpret_cast<thandle_t>(context_temp);
 //TIFF* tif = TIFFClientOpen("Tiff Image", mode, (thandle_t)context, tiff_read,
                       //      tiff_write, tiff_seek, tiff_close, tiff_get_size,
                        //     tiff_map, tiff_unmap);
                             
  if (tif) {
    tif->tif_fd = (int)(intptr_t)context;
  }
  return tif;


  sandbox.free_in_sandbox(string_T);
  sandbox.free_in_sandbox(mode_T);
  sandbox.free_in_sandbox(context_temp);
}

void TiffBGRA2RGBA(uint8_t* pBuf, int32_t pixel, int32_t spp) {
  for (int32_t n = 0; n < pixel; n++) {
    uint8_t tmp = pBuf[0];
    pBuf[0] = pBuf[2];
    pBuf[2] = tmp;
    pBuf += spp;
  }
}

}  // namespace

bool CTiffContext::InitDecoder(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox,
    const RetainPtr<IFX_SeekableReadStream>& file_ptr) {
  m_io_in = file_ptr;
  m_tif_ctx.reset(tiff_open(sandbox, this, "r"));
  return !!m_tif_ctx;
}

bool CTiffContext::LoadFrameInfo(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, int32_t frame,
                                 int32_t* width,
                                 int32_t* height,
                                 int32_t* comps,
                                 int32_t* bpc,
                                 CFX_DIBAttribute* pAttribute) {
  //invoke sandbox testing
  size_t size = sizeof(*m_tif_ctx.get());
  auto s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), 
  m_tif_ctx.get(), size);

  if(!sandbox.invoke_sandbox_function(TIFFSetDirectory, s, (uint16)frame).copy_and_verify([](int ret) {
    //TIFFSetDirectory returns answer from TIFFReadDirectory, which should only return 0 (fail) or success (1)
    assert(ret == 0 || ret == 1)
    return ret;
  }))
  auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote to this pointer, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
  // if (!TIFFSetDirectory(m_tif_ctx.get(), (uint16)frame))
    // return false;

  uint32_t tif_width = 0;
  uint32_t tif_height = 0;
  uint16_t tif_comps = 0;
  uint16_t tif_bpc = 0;
  uint32_t tif_rps = 0;

  auto tif_width_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  auto tif_height_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  auto tif_comps_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  auto tif_bpc_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  auto tif_rps_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  //invoke sandbox
  
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s,  TIFFTAG_IMAGEWIDTH, tif_width_ptr);
  tif_width = *tif_width_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);

  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_IMAGELENGTH, tif_height_ptr);
  tif_height = *tif_height_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);

  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_SAMPLESPERPIXEL, tif_comps_ptr);
  tif_comps = *tif_comps_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);

  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_BITSPERSAMPLE, tif_bpc_ptr);
  tif_bpc = *tif_bpc_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);

  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_ROWSPERSTRIP, tif_rps_ptr);
  tif_rps = *tif_rps_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
    /*
  TIFFGetField(m_tif_ctx.get(), TIFFTAG_IMAGEWIDTH, &tif_width);
  TIFFGetField(m_tif_ctx.get(), TIFFTAG_IMAGELENGTH, &tif_height);
  TIFFGetField(m_tif_ctx.get(), TIFFTAG_SAMPLESPERPIXEL, &tif_comps);
  TIFFGetField(m_tif_ctx.get(), TIFFTAG_BITSPERSAMPLE, &tif_bpc);
  TIFFGetField(m_tif_ctx.get(), TIFFTAG_ROWSPERSTRIP, &tif_rps);
*/
//invoke sandbox
  sandbox.free_in_sandbox(tif_width_ptr);
  sandbox.free_in_sandbox(tif_height_ptr);
  sandbox.free_in_sandbox(tif_comps_ptr);
  sandbox.free_in_sandbox(tif_bpc_ptr);
  

  
  

  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);

  uint16_t tif_resunit = 0;
  auto tif_resunit_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  auto bool1 = sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_RESOLUTIONUNIT, tif_resunit_ptr).copy_and_verify([tif_resunit_ptr](int ret){
   //2 valid cases: TIFFGetField returns 0 meaning field was not found,
   // or returns the value of the tag, with tif_resunit being set to the value of the field
   assert(ret == 0 || (ret == TIFFTAG_RESOLUTIONUNIT && (*tif_resunit_ptr == RESUNIT_NONE || *tif_resunit_ptr == RESUNIT_INCH || *tif_resunit_ptr == RESUNIT_CENTIMETER)))
    return ret;
  });
  //auto bool1 = (TIFFGetField(m_tif_ctx.get(), TIFFTAG_RESOLUTIONUNIT, &tif_resunit))
  tif_resunit = *tif_resunit_ptr
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
  if(bool1) {
  
  //if (TIFFGetField(m_tif_ctx.get(), TIFFTAG_RESOLUTIONUNIT, &tif_resunit)) {
     pAttribute->m_wDPIUnit =
        static_cast<CFX_DIBAttribute::ResUnit>(tif_resunit - 1);
  } else {
    pAttribute->m_wDPIUnit = CFX_DIBAttribute::kResUnitInch;
  }
  sandbox.free_in_sandbox(tif_resunit_ptr);
  float tif_xdpi = 0.0f;
  auto tif_xdpi_ptr = sandbox.malloc_in_sandbox<float>();
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_XRESOLUTION, tif_xdpi_ptr);
  tif_xpdi = *tif_xpdi_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  //invoke sandbox
 // TIFFGetField(m_tif_ctx.get(), TIFFTAG_XRESOLUTION, &tif_xdp

  if (tif_xdpi)// 
    pAttribute->m_nXDPI = static_cast<int32_t>(tif_xdpi + 0.5f);
  sandbox.free_in_sandbox(tif_xdpi_ptr);

  float tif_ydpi = 0.0f;
  auto tif_ydpi_ptr = sandbox.malloc_in_sandbox<float>();
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_YRESOLUTION, tif_ydpi_ptr);
  tif_ypdi = *tif_ypdi_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
  //invoke sandbox
  //TIFFGetField(m_tif_ctx.get(), TIFFTAG_YRESOLUTION, &tif_ydpi);
  if (tif_ydpi)
    pAttribute->m_nYDPI = static_cast<int32_t>(tif_ydpi + 0.5f);

  sandbox.free_in_sandbox(tif_ydpi_ptr);
  FX_SAFE_INT32 checked_width = tif_width;
  FX_SAFE_INT32 checked_height = tif_height;
  if (!checked_width.IsValid() || !checked_height.IsValid()) {
    sandbox.free_in_sandbox(tif_rps_ptr);
    return false;
  }
  *width = checked_width.ValueOrDie();
  *height = checked_height.ValueOrDie();
  *comps = tif_comps;
  *bpc = tif_bpc;
  if (tif_rps > tif_height) {
      tif_rps = tif_height;
    //invoke sandbox
    size = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size);
    rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
    sandbox.invoke_sandbox_function(TIFFSetField, s, TIFFTAG_ROWSPERSTRIP, tif_rps_ptr);
    tif_rps = *tif_rps_ptr;
    s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
    sandbox.free_in_sandbox(s);
 //   TIFFSetField(m_tif_ctx.get(), TIFFTAG_ROWSPERSTRIP, tif_rps);
  }
  
  sandbox.free_in_sandbox(tif_rps_ptr);
  return true;
}

bool CTiffContext::IsSupport(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap) const {
  //invoke sandbox
  auto size = sizeof(*m_tif_ctx.get());
  auto s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  auto bool1 = sandbox.invoke_sandbox_function(TIFFIsTiled, s).copy_and_verify([](int ret){
    //change
    return ret;
  });
   auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
    sandbox.free_in_sandbox(s);
  if(bool1)
//  if (TIFFIsTiled(m_tif_ctx.get()))
    return false;

  uint16_t photometric = 0;
  auto photometric_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  auto booe = sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_PHOTOMETRIC, photometric_ptr).copy_and_verify([photometric_ptr](int ret) {
      //change
      assert(ret == 0 || (ret == TIFFTAG_PHOTOMETRIC && (*photometric_ptr== PHOTOMETRIC_MINISWHITE || *photometric_ptr == PHOTOMETRIC_MINISBLACK || *photometric_ptr == PHOTOMETRIC_RBG || *photometric_ptr == PHOTOTMETRIC_PALETTE || *photometric_ptr == PHOTOTMETRIC_MASK || *photometric_ptr == PHOTOMETRIC_SEPARATED || *photometric_ptr == PHOTOTMETRIC_YCBCR || *photometric_ptr == PHOTOTMETRIC_CIELAB || *photometric_ptr== PHOTOTMETRIC_ICCLAB || *photometric_ptr == PHOTOTMETRIC_ITULAB || *photometric_ptr == PHOTOTMETRIC_LOGL || *photometric_ptr == PHOTOTMETRIC_LOGLUV)));
      return ret;
    });
     s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
      m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
      sandbox.free_in_sandbox(s);
          
  //invoke sandbox
  //if (!TIFFGetField(m_tif_ctx.get(), TIFFTAG_PHOTOMETRIC, &photometric))
    if(!booe) {
      sandbox.free_in_sandbox(photometric_ptr);
    return false;
    }
    photometric = *photometric_ptr;
    sandbox.free_in_sandbox(photometric_ptr);
  switch (pDIBitmap->GetBPP()) {
    case 1:
    case 8:
      if (photometric != PHOTOMETRIC_PALETTE) {
        return false;
      }
      break;
    case 24:
      if (photometric != PHOTOMETRIC_RGB) {
        return false;
      }
      break;
    default:
      return false;
  }
  uint16_t planarconfig = 0;
  auto planarconfig_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  auto boo2 = sandbox.invoke_sandbox_function(TIFFGetFieldDefaulted, s, TIFFTAG_PLANARCONFIG, planarconfig_ptr).copy_and_verify([](int ret) {
      //change
      return ret;
    });
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
    sandbox.free_in_sandbox(s);
  planarconfig = *planarconfig_ptr;
   sandbox.free_in_sandbox(planarconfig_ptr);
   if(!boo2) {
  //invoke sandbox (might be wrong)
  //if (!TIFFGetFieldDefaulted(m_tif_ctx.get(), TIFFTAG_PLANARCONFIG, &//planar_config)){
   
    return false;
    }
  
  return planarconfig != PLANARCONFIG_SEPARATE;
}

void CTiffContext::SetPalette(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                              uint16_t bps) {
  uint16_t* red_orig = nullptr;
  uint16_t* green_orig = nullptr;
  uint16_t* blue_orig = nullptr;
  auto red_orig_ptr = sandbox.malloc_in_sandbox<uint16_t*>();
  auto green_orig_ptr = sandbox.malloc_in_sandbox<uint16_t*>();
  auto blue_orig_ptr = sandbox.malloc_in_sandbox<uint16_t*>();
  auto size = sizeof(*m_tif_ctx.get());
  auto s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_COLORMAP, red_orig_ptr, green_orig_ptr, blue_orig_ptr);
  
  int32_t len = 1 << bps;
  red_orig = *red_orig_ptr;
  green_orig = *green_orig_ptr;
  blue_orig = *blue_orig_ptr;

  auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
   sandbox.free_in_sandbox(s);
  //invoke sandbox
  //TIFFGetField(m_tif_ctx.get(), TIFFTAG_COLORMAP, &red_orig, &green_orig,
       //        &blue_orig);
  for (int32_t i = pdfium::base::checked_cast<int32_t>((1L << bps) - 1); i >= 0;
       i--) {
#define CVT(x) ((uint16_t)((x) >> 8))
    red_orig[i] = CVT(red_orig[i]);
    green_orig[i] = CVT(green_orig[i]);
    blue_orig[i] = CVT(blue_orig[i]);
#undef CVT
  }
  
  for (int32_t index = 0; index < len; index++) {
    uint32_t r = red_orig[index] & 0xFF;
    uint32_t g = green_orig[index] & 0xFF;
    uint32_t b = blue_orig[index] & 0xFF;
    uint32_t color = (uint32_t)b | ((uint32_t)g << 8) | ((uint32_t)r << 16) |
                     (((uint32)0xffL) << 24);
    pDIBitmap->SetPaletteArgb(index, color);
  }
  sandbox.free_in_sandbox(red_orig_ptr);
  sandbox.free_in_sandbox(green_orig_ptr);
  sandbox.free_in_sandbox(blue_orig_ptr);
}

bool CTiffContext::Decode1bppRGB(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                                 int32_t height,
                                 int32_t width,
                                 uint16_t bps,
                                 uint16_t spp) {
  if (pDIBitmap->GetBPP() != 1 || spp != 1 || bps != 1 ||
      !IsSupport(sandbox, pDIBitmap)) {
    return false;
  }
  SetPalette(sandbox, pDIBitmap, bps);
  //invoke sandbox
  auto size2 = sizeof(*m_tif_ctx.get());
  auto s = sandbox.malloc_in_sandbox<Tiff>(size2);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
  int32_t size = static_cast<int32_t>sandbox.invoke_sandbox_function(TIFFScanlineSize, s).copy_and_verify([](tmsize_t ret) {
    //cannot read in a negative amount of lines
    assert(ret >= 0)
    return ret;
  });
  auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
  sandbox.free_in_sandbox(s);
  //int32_t size = static_cast<int32_t>(TIFFScanlineSize(m_tif_ctx.get()));
  uint8_t* buf = (uint8_t*)_TIFFmalloc(size);
  //invoke sandbox
  if (!buf) {
    // invoke sandbox
    size2 = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size2);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
    auto ans = sandbox.invoke_sandbox_function(TIFFFileName, s).copy_and_verify_string([](std::unique_ptr<char[]> val) {
      //file name cannot be null at this point
      assert(val != nullptr)
      return val;
    });
       s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
  sandbox.free_in_sandbox(s);

    auto ans_size = strlen(ans.get()) + 1;
    auto ans_T = sandbox.malloc_in_sandbox<char>(ans_size);
    std::strncpy(ans_T.unverified_safe_pointer_because(ans_size, "writing to memory"), ans.get(), ans_size);
    
    const char* string =  "No space for scanline buffer";
    auto size_1= strlen(string) + 1;
    auto string_T = sandbox.malloc_in_sandbox<char>(size_1);
    std::strncpy(string_T.unverified_safe_pointer_because(size_1, "writing to memory"), string, size_1);

    sandbox.invoke_sandbox_function(TIFFError, ans_T, string_T);
  sandbox.free_in_sandbox(ans_T);
  sandbox.free_in_sandbox(string_T);
   // TIFFError(TIFFFileName(m_tif_ctx.get()), "No space for scanline buffer");
    return false;
  }
 // rlbox::tainted<uint8_t*, rlbox::rlbox_wasm2c_sandbox> buf_T = buf;
  for (int32_t row = 0; row < height; row++) {
    uint8_t* bitMapbuffer = pDIBitmap->GetWritableScanline(row).data();
    //invoke sandbox
    size2 = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size2);
    rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
    sandbox.invoke_sandbox_function(TIFFReadScanline, s, buf_T, row, 0);
    s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
    sandbox.free_in_sandbox(s);
    //TIFFReadScanline(m_tif_ctx.get(), buf, row, 0);
    for (int32_t j = 0; j < size; j++) {
      bitMapbuffer[j] = buf[j];
    }
  }
  _TIFFfree(buf);


  return true;
}

bool CTiffContext::Decode8bppRGB(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                                 int32_t height,
                                 int32_t width,
                                 uint16_t bps,
                                 uint16_t spp) {
  if (pDIBitmap->GetBPP() != 8 || spp != 1 || (bps != 4 && bps != 8) ||
      !IsSupport(sandbox, pDIBitmap)) {
    return false;
  }
  SetPalette(sandbox, pDIBitmap, bps);
 //invoke sandbox
  auto size2 = sizeof(*m_tif_ctx.get());
  auto s = sandbox.malloc_in_sandbox<Tiff>(size2);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
  int32_t size = static_cast<int32_t>sandbox.invoke_sandbox_function(TIFFScanlineSize, s).copy_and_verify([](tmsize_t ret) {
    //cannot read - amount of lines
    assert(ret >= 0)
    return ret;
  });
  auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
  sandbox.free_in_sandbox(s);
  //int32_t size = static_cast<int32_t>(TIFFScanlineSize(m_tif_ctx.get()));
  uint8_t* buf = (uint8_t*)_TIFFmalloc(size);
  if (!buf) {
   size2 = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size2);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
    auto ans = sandbox.invoke_sandbox_function(TIFFFileName, s).copy_and_verify_string([](std::unique_ptr<char[]> val) {
      //at this point, name of file cannot be null 
      assert(val != nullptr)
      return val;
    });
       s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
  sandbox.free_in_sandbox(s);

    auto ans_size = strlen(ans.get()) + 1;
    auto ans_T = sandbox.malloc_in_sandbox<char>(ans_size);
    std::strncpy(ans_T.unverified_safe_pointer_because(ans_size, "writing to memory"), ans.get(), ans_size);
    
    const char* string =  "No space for scanline buffer";
    auto size_1= strlen(string) + 1;
    auto string_T = sandbox.malloc_in_sandbox<char>(size_1);
    std::strncpy(string_T.unverified_safe_pointer_because(size_1, "writing to memory"), string, size_1);

    sandbox.invoke_sandbox_function(TIFFError, ans_T, string_T);
  sandbox.free_in_sandbox(ans_T);
  sandbox.free_in_sandbox(string_T);
    //TIFFError(TIFFFileName(m_tif_ctx.get()), "No space for scanline buffer");
    return false;
  }
  //rlbox::tainted<uint8_t*, rlbox::rlbox_wasm2c_sandbox> buf_T = buf;
  for (int32_t row = 0; row < height; row++) {
    uint8_t* bitMapbuffer = pDIBitmap->GetWritableScanline(row).data();
    //invoke sandbox
        size2 = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size2);
    rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
    //sandbox.invoke_sandbox_function(TIFFReadScanline, s, buf_T, row, 0);
    s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
    sandbox.free_in_sandbox(s);
    
    //TIFFReadScanline(m_tif_ctx.get(), buf, row, 0);
    for (int32_t j = 0; j < size; j++) {
      switch (bps) {
        case 4:
          bitMapbuffer[2 * j + 0] = (buf[j] & 0xF0) >> 4;
          bitMapbuffer[2 * j + 1] = (buf[j] & 0x0F) >> 0;
          break;
        case 8:
          bitMapbuffer[j] = buf[j];
          break;
      }
    }
  }
  _TIFFfree(buf);

  return true;
}

bool CTiffContext::Decode24bppRGB(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox,const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                                  int32_t height,
                                  int32_t width,
                                  uint16_t bps,
                                  uint16_t spp) {
  if (pDIBitmap->GetBPP() != 24 || !IsSupport(sandbox, pDIBitmap))
    return false;
  //invoke sandbox
  auto size2 = sizeof(*m_tif_ctx.get());
  auto s = sandbox.malloc_in_sandbox<Tiff>(size2);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
  int32_t size = static_cast<int32_t>sandbox.invoke_sandbox_function(TIFFScanlineSize, s).copy_and_verify([](tmsize_t ret) {
    //cannot read in a negative amount of lines
    assert(ret >= 0)
    return ret;
  });
  auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
  sandbox.free_in_sandbox(s);
 // int32_t size = static_cast<int32_t>(TIFFScanlineSize(m_tif_ctx.get()));
  uint8_t* buf = (uint8_t*)_TIFFmalloc(size);
  if (!buf) {
   size2 = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size2);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
    auto ans = sandbox.invoke_sandbox_function(TIFFFileName, s).copy_and_verify_string([](std::unique_ptr<char[]> val) {
      //file name cannot be null at this point
    assert(val != nullptr)
      return val;
    });
       s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
  sandbox.free_in_sandbox(s);

    auto ans_size = strlen(ans.get()) + 1;
    auto ans_T = sandbox.malloc_in_sandbox<char>(ans_size);
    std::strncpy(ans_T.unverified_safe_pointer_because(ans_size, "writing to memory"), ans.get(), ans_size);
    
    const char* string =  "No space for scanline buffer";
    auto size_1= strlen(string) + 1;
    auto string_T = sandbox.malloc_in_sandbox<char>(size_1);
    std::strncpy(string_T.unverified_safe_pointer_because(size_1, "writing to memory"), string, size_1);

    sandbox.invoke_sandbox_function(TIFFError, ans_T, string_T);
  sandbox.free_in_sandbox(ans_T);
  sandbox.free_in_sandbox(string_T);
 // TIFFError(TIFFFileName(m_tif_ctx.get()), "No space for scanline buffer");  
    return false;
  }
 // rlbox::tainted<uint8_t*, rlbox::rlbox_wasm2c_sandbox> buf_T = buf;
  for (int32_t row = 0; row < height; row++) {
    uint8_t* bitMapbuffer = pDIBitmap->GetWritableScanline(row).data();
    //invoke sandbox
    size2 = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size2);
    rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size2, "writing to memory"), m_tif_ctx.get(), size2);
    sandbox.invoke_sandbox_function(TIFFReadScanline, s, buf_T, row, 0);
    s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp))
    sandbox.free_in_sandbox(s);
 
   // TIFFReadScanline(m_tif_ctx.get(), buf, row, 0);
    for (int32_t j = 0; j < size - 2; j += 3) {
      bitMapbuffer[j + 0] = buf[j + 2];
      bitMapbuffer[j + 1] = buf[j + 1];
      bitMapbuffer[j + 2] = buf[j + 0];
    }
  }
  _TIFFfree(buf);

  return true;
}

bool CTiffContext::Decode(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox, const RetainPtr<CFX_DIBitmap>& pDIBitmap) {
  uint32_t img_width = pDIBitmap->GetWidth();
  uint32_t img_height = pDIBitmap->GetHeight();

  uint32_t height = 0;

   auto height_ptr = sandbox.malloc_in_sandbox<uint32_t>();
  //invoke sandbox
  auto size = sizeof(*m_tif_ctx.get());
  auto s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);

  uint32_t width = 0;
  auto width_ptr = sandbox.malloc_in_sandbox<uint32_t>();
  //TIFFGetField(m_tif_ctx.get(), TIFFTAG_IMAGEWIDTH, &width);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_IMAGEWIDTH, width_ptr);
  width = *width_ptr;
  
  auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_IMAGELENGTH, height_ptr);
  height = *height_ptr;
  auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
  sandbox.free_in_sandbox(width_ptr);
  sandbox.free_in_sandbox(height_ptr);
  //TIFFGetField(m_tif_ctx.get(), TIFFTAG_IMAGEWIDTH, &width);
  //TIFFGetField(m_tif_ctx.get(), TIFFTAG_IMAGELENGTH, &height);
  if (img_width != width || img_height != height)
    return false;

  if (pDIBitmap->GetBPP() == 32) {
    uint16_t rotation = ORIENTATION_TOPLEFT;
    //invoke sandbox
     auto rotation_ptr = sandbox.malloc_in_sandbox<uint16_t>();
    size = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size);
    rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
    sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_ORIENTATION, rotation_ptr);
    rotation = *rotation_ptr;
    auto s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
     sandbox.free_in_sandbox(s);
    //TIFFGetField(m_tif_ctx.get(), TIFFTAG_ORIENTATION, &rotation);
    //invoke sandbox
    uint32* temp = (uint32*)pDIBitmap->GetBuffer();
  auto temp_T= sandbox.malloc_in_sandbox<uint32_t>(sizeof(temp));
  rlbox::memcpy(sandbox, temp_T, temp, sizeof(temp));
   size = sizeof(*m_tif_ctx.get());
    s = sandbox.malloc_in_sandbox<Tiff>(size);
    rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  auto bool1 = sandbox.invoke_sandbox_function(TIFFReadRGBAImageOriented, s, img_width, img_height, temp_T, rotation, 1).copy_and_verify([](int ret){
   //change
   return ret;
 });
 s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
    m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
     sandbox.free_in_sandbox(s);
 if(bool1) {
  // if (TIFFReadRGBAImageOriented(m_tif_ctx.get(), img_width, img_height,
  //                              (uint32*)pDIBitmap->GetBuffer(), rotation,
  //                               1)) {
      for (uint32_t row = 0; row < img_height; row++) {
        uint8_t* row_buf = pDIBitmap->GetWritableScanline(row).data();
        TiffBGRA2RGBA(row_buf, img_width, 4);
      }
sandbox.free_in_sandbox(rotation_ptr);
//sandbox.free_in_sandbox(temp_T);
      return true;
    }
  }
  uint16_t spp = 0;
  uint16_t bps = 0;

  auto spp_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  auto bps_ptr = sandbox.malloc_in_sandbox<uint16_t>();
  //invoke sandbox
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_SAMPLESPERPIXEL, spp_ptr);
  spp = *spp_ptr;
  s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
  size = sizeof(*m_tif_ctx.get());
  s = sandbox.malloc_in_sandbox<Tiff>(size);
  rlbox::memcpy(sandbox, s.unverified_safe_pointer_because(size, "writing to memory"), m_tif_ctx.get(), size);
  sandbox.invoke_sandbox_function(TIFFGetField, s, TIFFTAG_BITSPERSAMPLE, bps_ptr);
  bps = *bps_ptr;
   s_temp = s.unverified_safe_pointer_because(size, "only internal function wrote, no passed in arguments used");
  m_tif_ctx.reset(reinterpret_cast<TIFF*>(s_temp));
  sandbox.free_in_sandbox(s);
  sandbox.free_in_sandbox(spp_ptr);
  sandbox.free_in_sandbox(bps_ptr);
 // TIFFGetField(m_tif_ctx.get(), TIFFTAG_SAMPLESPERPIXEL, &spp);
 // TIFFGetField(m_tif_ctx.get(), TIFFTAG_BITSPERSAMPLE, &bps);
  FX_SAFE_UINT32 safe_bpp = bps;
  safe_bpp *= spp;
  if (!safe_bpp.IsValid())
    return false;
  uint32_t bpp = safe_bpp.ValueOrDie();
  if (bpp == 1)
    return Decode1bppRGB(sandbox, pDIBitmap, height, width, bps, spp);
  if (bpp <= 8)
    return Decode8bppRGB(sandbox, pDIBitmap, height, width, bps, spp);
  if (bpp <= 24)
    return Decode24bppRGB(sandbox, pDIBitmap, height, width, bps, spp);
  return false;
}

namespace fxcodec {

// static
std::unique_ptr<ProgressiveDecoderIface::Context> TiffDecoder::CreateDecoder(rlbox::rlbox_sandbox<rlbox_noop_sandbox> sandbox,
    const RetainPtr<IFX_SeekableReadStream>& file_ptr) {
  auto pDecoder = std::make_unique<CTiffContext>();
  if (!pDecoder->InitDecoder(sandbox, file_ptr))
    return nullptr;

  return pDecoder;
}

// static
bool TiffDecoder::LoadFrameInfo(rlbox::rlbox_sandbox<rlbox_noop_sandbox>  sandbox, ProgressiveDecoderIface::Context* pContext,
                                int32_t frame,
                                int32_t* width,
                                int32_t* height,
                                int32_t* comps,
                                int32_t* bpc,
                                CFX_DIBAttribute* pAttribute) {
  DCHECK(pAttribute);

  auto* ctx = static_cast<CTiffContext*>(pContext);
  return ctx->LoadFrameInfo(sandbox, frame, width, height, comps, bpc, pAttribute);
}

// static
bool TiffDecoder::Decode(rlbox::rlbox_sandbox<rlbox_noop_sandbox>  sandbox, ProgressiveDecoderIface::Context* pContext,
                         const RetainPtr<CFX_DIBitmap>& pDIBitmap) {
  auto* ctx = static_cast<CTiffContext*>(pContext);
  return ctx->Decode(sandbox, pDIBitmap);
}

}  // namespace fxcodec
