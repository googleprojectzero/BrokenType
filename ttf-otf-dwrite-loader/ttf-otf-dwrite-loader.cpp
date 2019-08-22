/////////////////////////////////////////////////////////////////////////
//
// Author: Mateusz Jurczyk (mjurczyk@google.com)
//
// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <Windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_3.h>
#include <comdef.h>

#include "config.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#ifndef IFR
#define IFR(expr) do {hr = (expr); _ASSERT(SUCCEEDED(hr)); if (FAILED(hr)) return(hr);} while(0)
#endif

FLOAT ConvertPointSizeToDIP(FLOAT points) {
  return (points / 72.0f) * 96.0f;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
  HRESULT hr = S_OK;
  IDWriteFactory *pDWriteFactory;
  IDWriteGdiInterop *pGdiInterop;
  IDWriteBitmapRenderTarget *pBitmapRenderTarget;
  IDWriteRenderingParams *pRenderingParams;

  if (argc != 2) {
    wprintf(L"Usage: %s <font path>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Initialize DirectWrite.
  IFR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                          reinterpret_cast<IUnknown**>(&pDWriteFactory)));
  IFR(pDWriteFactory->GetGdiInterop(&pGdiInterop));
  IFR(pGdiInterop->CreateBitmapRenderTarget(GetDC(NULL), RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT, &pBitmapRenderTarget));
  IFR(pDWriteFactory->CreateRenderingParams(&pRenderingParams));

  LPCWSTR szFontPath = argv[1];
  BOOL bSuccess = TRUE;
  IDWriteFontFile *pFontFile = NULL;
  do {
    hr = pDWriteFactory->CreateFontFileReference(szFontPath, NULL, &pFontFile);
    if (FAILED(hr)) {
      wprintf(L"[-] IDWriteFactory::CreateFontFileReference failed with code %x\n", hr);
      bSuccess = FALSE;
      break;
    }

    BOOL isSupportedFontType = FALSE;
    DWRITE_FONT_FILE_TYPE fontFileType;
    DWRITE_FONT_FACE_TYPE fontFaceType;
    UINT32 numberOfFaces;
    hr = pFontFile->Analyze(&isSupportedFontType, &fontFileType, &fontFaceType, &numberOfFaces);
    if (FAILED(hr)) {
      wprintf(L"[-] IDWriteFontFile::Analyze failed with code %x\n", hr);
      bSuccess = FALSE;
      break;
    }

    if (!isSupportedFontType) {
      wprintf(L"[-] The input font with file type %d and face type %d isn't supported by DirectWrite\n", fontFileType, fontFaceType);
      bSuccess = FALSE;
      break;
    }

    wprintf(L"[+] Input font is supported, with file type %d, face type %d and number of faces %d\n",
            fontFileType, fontFaceType, numberOfFaces);

    for (UINT32 face_it = 0; face_it < numberOfFaces; face_it++) {
      IDWriteFontFile *pFontFiles[] = { pFontFile };
      IDWriteFontFace *pFontFace = NULL;

      hr = pDWriteFactory->CreateFontFace(fontFaceType, 1, pFontFiles, face_it, DWRITE_FONT_SIMULATIONS_NONE, &pFontFace);
      if (FAILED(hr)) {
        wprintf(L"[-] IDWriteFactory::CreateFontFace(faceIndex=%u) failed with code %x\n", face_it, hr);
        bSuccess = FALSE;
          
        // Don't bail out at this point and instead continue processing other font faces.
        continue;
      }

      IDWriteFontFace1 *pFontFace1 = static_cast<IDWriteFontFace1 *>(pFontFace);

      DWRITE_FONT_METRICS fontMetrics;
      pFontFace->GetMetrics(&fontMetrics);

      UINT32 actualRangeCount;
      hr = pFontFace1->GetUnicodeRanges(0, NULL, &actualRangeCount);
      if (SUCCEEDED(hr)) {
        pFontFace->Release();
        continue;
      } else if (hr != E_NOT_SUFFICIENT_BUFFER) {
        wprintf(L"[-] IDWriteFontFace1::GetUnicodeRanges(faceIndex=%u) failed with code %x\n", face_it, hr);
        bSuccess = FALSE;
        pFontFace->Release();
        continue;
      }

      DWRITE_UNICODE_RANGE *pRanges = new DWRITE_UNICODE_RANGE[actualRangeCount];

      hr = pFontFace1->GetUnicodeRanges(actualRangeCount, pRanges, &actualRangeCount);
      if (FAILED(hr)) {
        wprintf(L"[-] IDWriteFontFace1::GetUnicodeRanges(faceIndex=%u) failed second time with code %d\n", face_it, hr);
        bSuccess = FALSE;
          
        delete[] pRanges;
        pFontFace->Release();
        continue;
      }

      wprintf(L"[+] Loaded %u unicode ranges from the input font\n", actualRangeCount);

      BitBlt(pBitmapRenderTarget->GetMemoryDC(), 0, 0, RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT, NULL, 0, 0, WHITENESS);

      UINT32 code_points[DISPLAYED_GLYPHS_COUNT];
      UINT16 glyph_indices[DISPLAYED_GLYPHS_COUNT];
      DWRITE_GLYPH_METRICS glyph_metrics[DISPLAYED_GLYPHS_COUNT];
      DWORD char_count = 0;
      for (UINT32 range_it = 0; range_it < actualRangeCount; range_it++) {
        for (UINT32 ch = pRanges[range_it].first; ch <= pRanges[range_it].last; ch++) {
          code_points[char_count++] = ch;

          if (char_count >= DISPLAYED_GLYPHS_COUNT ||
              (range_it == actualRangeCount - 1 && ch == pRanges[range_it].last && char_count > 0)) {

            hr = pFontFace1->GetGlyphIndices(code_points, char_count, glyph_indices);
            if (SUCCEEDED(hr)) {
              hr = pFontFace->GetDesignGlyphMetrics(glyph_indices, char_count, glyph_metrics, /*isSideways=*/FALSE);
              if (FAILED(hr)) {
                wprintf(L"[+] IDWriteFontFace::GetDesignGlyphMetrics([%x .. %x]) failed with code %x\n", code_points[0], code_points[char_count - 1], hr);
                bSuccess = FALSE;
              }

              CONST FLOAT point_sizes[] = { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 22.0f, 24.0f, 26.0f, 28.0f, 36.0f, 48.0f, 72.0f };
              DWRITE_GLYPH_RUN glyphRun = { pFontFace, ConvertPointSizeToDIP(12.0f), char_count, glyph_indices, NULL, NULL, FALSE, 0 };

              for (FLOAT point_size : point_sizes) {
                glyphRun.fontEmSize = ConvertPointSizeToDIP(point_size);
                CONST COLORREF color_ref = RGB(point_size * 2, point_size * 2, point_size * 2);
                hr = pBitmapRenderTarget->DrawGlyphRun(point_size, point_size, DWRITE_MEASURING_MODE_NATURAL, &glyphRun, pRenderingParams, color_ref);

                if (FAILED(hr)) {
                  bSuccess = FALSE;
                }
              }
            } else {
              wprintf(L"[-] IDWriteFontFace1::GetGlyphIndices failed with code %x\n", hr);
            }

            char_count = 0;

            BitBlt(GetDC(NULL), 0, 0, RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT, pBitmapRenderTarget->GetMemoryDC(), 0, 0, SRCCOPY | NOMIRRORBITMAP);
          }
        }
      }

      wprintf(L"[+] Font processing completed\n");

      delete[] pRanges;
      pFontFace->Release();
    }
  } while (0);

  if (bSuccess) {
    wprintf(L"[+] Font displayed without errors\n");
  }

  if (pFontFile != NULL) {
    pFontFile->Release();
  }

  pRenderingParams->Release();
  pBitmapRenderTarget->Release();
  pGdiInterop->Release();
  pDWriteFactory->Release();

	return 0;
}

