// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_COLORSPACE_H_
#define CORE_FPDFAPI_PAGE_CPDF_COLORSPACE_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <set>
#include <vector>

#include "core/fpdfapi/page/cpdf_pattern.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/observed_ptr.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "third_party/base/span.h"

class CPDF_Array;
class CPDF_Document;
class CPDF_IndexedCS;
class CPDF_Object;
class CPDF_PatternCS;

constexpr size_t kMaxPatternColorComps = 16;

class PatternValue {
 public:
  PatternValue();
  PatternValue(const PatternValue& that);
  ~PatternValue();

  void SetComps(pdfium::span<const float> comps);
  pdfium::span<const float> GetComps() const {
    // TODO(tsepez): update span.h from base for implicit std::array ctor.
    return {m_Comps.data(), m_Comps.size()};
  }

  CPDF_Pattern* GetPattern() const { return m_pRetainedPattern.Get(); }
  void SetPattern(const RetainPtr<CPDF_Pattern>& pPattern) {
    m_pRetainedPattern = pPattern;
  }

 private:
  RetainPtr<CPDF_Pattern> m_pRetainedPattern;
  std::array<float, kMaxPatternColorComps> m_Comps;
};

class CPDF_ColorSpace : public Retainable, public Observable {
 public:
  enum class Family {
    kUnknown = 0,
    kDeviceGray = 1,
    kDeviceRGB = 2,
    kDeviceCMYK = 3,
    kCalGray = 4,
    kCalRGB = 5,
    kLab = 6,
    kICCBased = 7,
    kSeparation = 8,
    kDeviceN = 9,
    kIndexed = 10,
    kPattern = 11,
  };

  static RetainPtr<CPDF_ColorSpace> GetStockCS(Family family);
  static RetainPtr<CPDF_ColorSpace> GetStockCSForName(const ByteString& name);
  static RetainPtr<CPDF_ColorSpace> Load(
      CPDF_Document* pDoc,
      const CPDF_Object* pObj,
      std::set<const CPDF_Object*>* pVisited);

  static RetainPtr<CPDF_ColorSpace> AllocateColorSpaceForID(
      CPDF_Document* pDocument,
      uint32_t family_id);

  static uint32_t ComponentsForFamily(Family family);
  static bool IsValidIccComponents(int components);

  const CPDF_Array* GetArray() const { return m_pArray.Get(); }

  // Should only be called if this colorspace is not a pattern.
  std::vector<float> CreateBufAndSetDefaultColor() const;

  uint32_t CountComponents() const;
  Family GetFamily() const { return m_Family; }
  bool IsSpecial() const {
    return GetFamily() == Family::kSeparation ||
           GetFamily() == Family::kDeviceN || GetFamily() == Family::kIndexed ||
           GetFamily() == Family::kPattern;
  }

  // Use CPDF_Pattern::GetPatternRGB() instead of GetRGB() for patterns.
  virtual bool GetRGB(pdfium::span<const float> pBuf,
                      float* R,
                      float* G,
                      float* B) const = 0;

  virtual void GetDefaultValue(int iComponent,
                               float* value,
                               float* min,
                               float* max) const;

  virtual void TranslateImageLine(pdfium::span<uint8_t> dest_span,
                                  pdfium::span<const uint8_t> src_span,
                                  int pixels,
                                  int image_width,
                                  int image_height,
                                  bool bTransMask) const;
  virtual void EnableStdConversion(bool bEnabled);
  virtual bool IsNormal() const;

  // Returns `this` as a CPDF_PatternCS* if `this` is a pattern.
  virtual const CPDF_PatternCS* AsPatternCS() const;

  // Returns `this` as a CPDF_IndexedCS* if `this` is indexed.
  virtual const CPDF_IndexedCS* AsIndexedCS() const;

 protected:
  explicit CPDF_ColorSpace(Family family);
  ~CPDF_ColorSpace() override;

  // Returns the number of components, or 0 on failure.
  virtual uint32_t v_Load(CPDF_Document* pDoc,
                          const CPDF_Array* pArray,
                          std::set<const CPDF_Object*>* pVisited) = 0;

  // Stock colorspaces are not loaded normally. This initializes their
  // components count.
  void SetComponentsForStockCS(uint32_t nComponents);

  bool IsStdConversionEnabled() const { return m_dwStdConversion != 0; }

  RetainPtr<const CPDF_Array> m_pArray;
  const Family m_Family;

 private:
  friend class CPDF_CalGray_TranslateImageLine_Test;
  friend class CPDF_CalRGB_TranslateImageLine_Test;

  static RetainPtr<CPDF_ColorSpace> AllocateColorSpace(
      ByteStringView bsFamilyName);

  uint32_t m_dwStdConversion = 0;
  uint32_t m_nComponents = 0;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_COLORSPACE_H_
