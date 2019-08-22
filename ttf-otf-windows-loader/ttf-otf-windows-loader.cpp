/////////////////////////////////////////////////////////////////////////
//
// Author: Mateusz Jurczyk (mjurczyk@google.com)
//
// Copyright 2018 Google LLC
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

#include <Windows.h>
#include <Windowsx.h>
#include <usp10.h>

#include <cstdio>

#include "config.h"

#pragma comment(lib, "Usp10.lib")

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

//
// Undocumented definitions required to use the gdi32!GetFontResourceInfoW function.
//
typedef BOOL(WINAPI *PGFRI)(LPCWSTR, LPDWORD, LPVOID, DWORD);
#define QFR_LOGFONT (2)

static BOOL GetLogfonts(LPCWSTR szFontPath, LPLOGFONTW *lpLogfonts, LPDWORD lpdwFonts) {
  // Unload all instances of fonts with this path, in case there are any leftovers in the system.
  while (RemoveFontResourceW(szFontPath)) {}

  // Load the font file into the system temporarily.
  DWORD dwFontsLoaded = AddFontResourceW(szFontPath);
  if (dwFontsLoaded == 0) {
    wprintf(L"[-] AddFontResourceW() failed.\n");
    return FALSE;
  }

  *lpdwFonts = dwFontsLoaded;
  *lpLogfonts = (LPLOGFONTW)HeapAlloc(GetProcessHeap(), 0, dwFontsLoaded * sizeof(LOGFONTW));
  if (*lpLogfonts == NULL) {
    wprintf(L"[-] HeapAlloc(%u) failed.\n", dwFontsLoaded * sizeof(LOGFONTW));
    RemoveFontResourceW(szFontPath);
    return FALSE;
  }

  // Resolve the GDI32!GetFontResourceInfoW symbol.
  HINSTANCE hGdi32 = GetModuleHandleA("gdi32.dll");
  PGFRI GetFontResourceInfo = (PGFRI)GetProcAddress(hGdi32, "GetFontResourceInfoW");

  DWORD cbBuffer = dwFontsLoaded * sizeof(LOGFONTW);
  if (!GetFontResourceInfo(szFontPath, &cbBuffer, *lpLogfonts, QFR_LOGFONT)) {
    wprintf(L"[-] GetFontResourceInfoW() failed.\n");
    HeapFree(GetProcessHeap(), 0, *lpLogfonts);
    RemoveFontResourceW(szFontPath);
    return FALSE;
  }

  // Unload the font.
  RemoveFontResourceW(szFontPath);
  return TRUE;
}

static VOID TestUniscribe(HDC hdc, LPGLYPHSET lpGlyphset) {
  SCRIPT_CACHE sc = NULL;

  // Get font height.
  long tmHeight;
  ScriptCacheGetHeight(hdc, &sc, &tmHeight);

  // Get font properties.
  SCRIPT_FONTPROPERTIES fp;
  ScriptGetFontProperties(hdc, &sc, &fp);

  // Perform some operations (mostly in batches) over each supported glyph.
  WCHAR szTextBuffer[DISPLAYED_GLYPHS_COUNT];
  DWORD dwTextCount = 0;
  WORD *wOutGlyphs = (WORD *)HeapAlloc(GetProcessHeap(), 0, DISPLAYED_GLYPHS_COUNT * sizeof(WORD));

  for (DWORD i = 0; i < lpGlyphset->cRanges; i++) {
    for (LONG j = lpGlyphset->ranges[i].wcLow; j < lpGlyphset->ranges[i].wcLow + lpGlyphset->ranges[i].cGlyphs; j++) {
      szTextBuffer[dwTextCount++] = (WCHAR)j;

      // Test particular characters.
      ABC abc;
      ScriptGetGlyphABCWidth(hdc, &sc, (WORD)j, &abc);

      // Test characters in batches where possible.
      if (dwTextCount >= DISPLAYED_GLYPHS_COUNT) {
        // Test the usp10!ScriptGetCMap function.
        ScriptGetCMap(hdc, &sc, szTextBuffer, dwTextCount, 0, wOutGlyphs);

        dwTextCount = 0;
      }
    }
  }

  if (dwTextCount > 0) {
    // Test the usp10!ScriptGetCMap function.
    ScriptGetCMap(hdc, &sc, szTextBuffer, dwTextCount, 0, wOutGlyphs);
  }

  // Call some script/lang/feature-related APIs over each glyph.
  OPENTYPE_TAG *ScriptTags = (OPENTYPE_TAG *)HeapAlloc(GetProcessHeap(), 0, UNISCRIBE_MAX_TAGS * sizeof(OPENTYPE_TAG));
  OPENTYPE_TAG *LangTags = (OPENTYPE_TAG *)HeapAlloc(GetProcessHeap(), 0, UNISCRIBE_MAX_TAGS * sizeof(OPENTYPE_TAG));
  OPENTYPE_TAG *FeatureTags = (OPENTYPE_TAG *)HeapAlloc(GetProcessHeap(), 0, UNISCRIBE_MAX_TAGS * sizeof(OPENTYPE_TAG));
  int cScriptTags = 0, cLangTags = 0, cFeatureTags = 0;

  WORD *AlternateGlyphs = (WORD *)HeapAlloc(GetProcessHeap(), 0, MAX_ALTERNATE_GLYPHS * sizeof(WORD));
  int cAlternates;

  // Retrieve a list of available scripts in the font.
  if (ScriptGetFontScriptTags(hdc, &sc, NULL, UNISCRIBE_MAX_TAGS, ScriptTags, &cScriptTags) == 0) {
    for (int ScriptTag = 0; ScriptTag < cScriptTags; ScriptTag++) {

      // Retrieve a list of language tags for the specified script tag.
      if (ScriptGetFontLanguageTags(hdc, &sc, NULL, ScriptTags[ScriptTag], UNISCRIBE_MAX_TAGS, LangTags, &cLangTags) == 0) {
        for (int LangTag = 0; LangTag < cLangTags; LangTag++) {

          // Retrieve a list of typographic features for the defined writing system.
          if (ScriptGetFontFeatureTags(hdc, &sc, NULL, ScriptTags[ScriptTag], LangTags[LangTag], UNISCRIBE_MAX_TAGS, FeatureTags, &cFeatureTags) == 0) {
            for (int FeatureTag = 0; FeatureTag < cFeatureTags; FeatureTag++) {

              // Iterate through all glyphs in the font.
              for (DWORD i = 0; i < lpGlyphset->cRanges; i++) {
                for (LONG j = lpGlyphset->ranges[i].wcLow; j < lpGlyphset->ranges[i].wcLow + lpGlyphset->ranges[i].cGlyphs; j++) {

                  // Test usp10!ScriptGetFontAlternateGlyphs.
                  if (ScriptGetFontAlternateGlyphs(hdc, &sc, NULL, ScriptTags[ScriptTag], LangTags[LangTag], FeatureTags[FeatureTag],
                                                   (WORD)j, MAX_ALTERNATE_GLYPHS, AlternateGlyphs, &cAlternates) == 0) {

                    for (int alt_glyph_id = 1; alt_glyph_id < cAlternates; alt_glyph_id++) {
                      WORD wOutGlyphId;

                      // Test usp10!ScriptSubstituteSingleGlyph.
                      ScriptSubstituteSingleGlyph(hdc, &sc, NULL, ScriptTags[ScriptTag], LangTags[LangTag], FeatureTags[FeatureTag],
                                                  alt_glyph_id, (WORD)j, &wOutGlyphId);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  ScriptFreeCache(&sc);

  HeapFree(GetProcessHeap(), 0, wOutGlyphs);
  HeapFree(GetProcessHeap(), 0, ScriptTags);
  HeapFree(GetProcessHeap(), 0, LangTags);
  HeapFree(GetProcessHeap(), 0, FeatureTags);
  HeapFree(GetProcessHeap(), 0, AlternateGlyphs);
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
  if (argc != 2) {
    wprintf(L"Usage: %s <font path>\n", argv[0]);
    return EXIT_FAILURE;
  }

  LPCWSTR szFontPath = argv[1];

  // Get screen coordinates.
  RECT screen_rect = { 0 };
  screen_rect.left = 0;
  screen_rect.top = 0;
  screen_rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
  screen_rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);

  // Reset the PRNG state.
  srand(0);

  do {
    // Get the logfont structures.
    LPLOGFONTW lpLogfonts = NULL;
    DWORD dwFonts = 0;

    if (GetLogfonts(szFontPath, &lpLogfonts, &dwFonts)) {
      wprintf(L"[+] Extracted %u logfonts.\n", dwFonts);
    } else {
      break;
    }

    // Load the font in the system from memory.
    int cFonts = AddFontResourceW(szFontPath);
    if (cFonts > 0) {
      wprintf(L"[+] Installed %d fonts.\n", cFonts);
    } else {
      wprintf(L"[-] AddFontResourceW() failed.\n");
      break;
    }

    HDC hDC = GetDC(NULL);
    SetGraphicsMode(hDC, GM_ADVANCED);
    SetMapMode(hDC, MM_TEXT);

    // Display all fonts from the input file.
    BOOL success = TRUE;
    for (DWORD font_it = 0; success && font_it < dwFonts; font_it++) {
      // Display the font in several different point sizes.
      CONST LONG point_sizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };

      for (unsigned int variation_it = 0; success && variation_it < ARRAY_SIZE(point_sizes) + 1; variation_it++) {
        wprintf(L"[+] Starting to test font %u / %d, variation %u / %d\n",
                font_it + 1, dwFonts, variation_it + 1, ARRAY_SIZE(point_sizes) + 1);

        HFONT hFont = NULL;
        if (variation_it == 0) {
          hFont = CreateFontIndirectW(&lpLogfonts[font_it]);
        } else {
          LOGFONTW lf;
          RtlCopyMemory(&lf, &lpLogfonts[font_it], sizeof(LOGFONTW));

          lf.lfHeight = -MulDiv(point_sizes[variation_it - 1], GetDeviceCaps(hDC, LOGPIXELSY), 72);
          lf.lfWeight = 0;
          lf.lfItalic = (rand() & 1);
          lf.lfUnderline = (rand() & 1);
          lf.lfStrikeOut = (rand() & 1);
          lf.lfQuality = (rand() % 6);

          hFont = CreateFontIndirectW(&lf);
        }

        if (hFont == NULL) {
          wprintf(L"[!]   CreateFontIndirectW() failed.\n");
          success = FALSE;
          DeleteFont(hFont);
          break;
        }

        // Select the font for the Device Context.
        SelectFont(hDC, hFont);

#if HARNESS_TEST_KERNING_PAIRS
        wprintf(L"[+]   Getting kerning pairs.\n");

        // Get the font's kerning pairs.
        DWORD nNumPairs = GetKerningPairs(hDC, 0, NULL);
        if (nNumPairs != 0) {
          LPKERNINGPAIR lpkrnpairs = (LPKERNINGPAIR)HeapAlloc(GetProcessHeap(), 0, nNumPairs * sizeof(KERNINGPAIR));

          if (GetKerningPairs(hDC, nNumPairs, lpkrnpairs) == 0) {
            wprintf(L"[!]   GetKerningPairs() failed.\n");
          }

          HeapFree(GetProcessHeap(), 0, lpkrnpairs);
        }
#endif  // HARNESS_TEST_KERNING_PAIRS

        wprintf(L"[+]   Getting unicode ranges.\n");

        // Get Unicode ranges available in the font.
        DWORD dwGlyphsetSize = GetFontUnicodeRanges(hDC, NULL);
        if (dwGlyphsetSize == 0) {
          wprintf(L"[!]   GetFontUnicodeRanges() failed.\n");
          success = FALSE;
          DeleteFont(hFont);
          break;
        }

        LPGLYPHSET lpGlyphset = (LPGLYPHSET)HeapAlloc(GetProcessHeap(), 0, dwGlyphsetSize);

        if (GetFontUnicodeRanges(hDC, lpGlyphset) == 0) {
          wprintf(L"[!]   GetFontUnicodeRanges() failed.\n");
          success = FALSE;
          HeapFree(GetProcessHeap(), 0, lpGlyphset);
          DeleteFont(hFont);
          break;
        }

#if HARNESS_TEST_DRAWTEXT
        WCHAR szTextBuffer[DISPLAYED_GLYPHS_COUNT + 1];
        DWORD dwTextCount = 0;
#endif  // HARNESS_TEST_DRAWTEXT

        wprintf(L"[+]   Getting glyph outlines and drawing them on screen.\n");

        for (DWORD i = 0; i < lpGlyphset->cRanges; i++) {
          for (LONG j = lpGlyphset->ranges[i].wcLow; j < lpGlyphset->ranges[i].wcLow + lpGlyphset->ranges[i].cGlyphs; j++) {
#if HARNESS_TEST_GLYPH_OUTLINE
            // Get the glyph outline in all available formats.
            CONST UINT glyph_formats[] = { GGO_BEZIER, GGO_BITMAP, GGO_GRAY2_BITMAP, GGO_GRAY4_BITMAP, GGO_GRAY8_BITMAP, GGO_NATIVE };
            for (UINT format : glyph_formats) {
              GLYPHMETRICS gm;
              MAT2 mat2 = { 0, 1, 0, 0, 0, 0, 0, 1 };

              DWORD cbBuffer = GetGlyphOutline(hDC, j, format, &gm, 0, NULL, &mat2);

              if (cbBuffer != GDI_ERROR) {
                LPVOID lpvBuffer = HeapAlloc(GetProcessHeap(), 0, cbBuffer);

                if (GetGlyphOutline(hDC, j, format, &gm, cbBuffer, lpvBuffer, &mat2) == GDI_ERROR) {
                  wprintf(L"[!]   GetGlyphOutline() failed for glyph %u.\n", j);
                }

                HeapFree(GetProcessHeap(), 0, lpvBuffer);
              }
            }
#endif  // HARNESS_TEST_GLYPH_OUTLINE

#if HARNESS_TEST_DRAWTEXT
            // Insert the glyph into current string to be displayed.
            szTextBuffer[dwTextCount++] = (WCHAR)j;
            if (dwTextCount >= DISPLAYED_GLYPHS_COUNT) {
              szTextBuffer[DISPLAYED_GLYPHS_COUNT] = L'\0';
              DrawTextW(hDC, szTextBuffer, -1, &screen_rect, DT_WORDBREAK | DT_NOCLIP);
              dwTextCount = 0;
            }
#endif  // HARNESS_TEST_DRAWTEXT
          }
        }

#if HARNESS_TEST_DRAWTEXT
        if (dwTextCount > 0) {
          szTextBuffer[dwTextCount] = L'\0';
          DrawTextW(hDC, szTextBuffer, -1, &screen_rect, DT_WORDBREAK | DT_NOCLIP);
        }
#endif  // HARNESS_TEST_DRAWTEXT

#if HARNESS_TEST_UNISCRIBE
        wprintf(L"[+]   Testing the Uniscribe user-mode library.\n");

        // Test some user-mode Windows Uniscribe functionality.
        TestUniscribe(hDC, lpGlyphset);
#endif  // HARNESS_TEST_UNISCRIBE

        HeapFree(GetProcessHeap(), 0, lpGlyphset);
        DeleteFont(hFont);
      }
    }

    // Remove the font from the system.
    ReleaseDC(NULL, hDC);
    RemoveFontResourceW(szFontPath);
  } while (0);

  return 0;
}

