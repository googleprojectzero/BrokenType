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

#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <fontsub.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <random>
#include <vector>

#pragma comment(lib, "fontsub.lib")

#ifdef _DEBUG
# define LOG(x) printf x;
#else
# define LOG(x)
#endif  // DEBUG

static void *CfpAllocproc(size_t Arg1) {
  void *ret = malloc(Arg1);
  LOG(("[+] malloc(0x%zx) ---> %p\n", Arg1, ret));
  return ret;
}

static void CfpFreeproc(void *Arg1) {
  LOG(("[+] free(%p)\n", Arg1));
  return free(Arg1);
}

static void *CfpReallocproc(void *Arg1, size_t Arg2) {
  void *ret = realloc(Arg1, Arg2);
  LOG(("[+] realloc(%p, 0x%zx) ---> %p\n", Arg1, Arg2, ret));
  return ret;
}

static bool ReadFileToString(const char *filename, std::string *buffer) {
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    return false;
  }

  fseek(f, 0, SEEK_END);
  const long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  bool success = true;

  char *tmp = (char *)malloc(size);
  if (tmp == NULL) {
    success = false;
  }

  if (success) {
    success = (fread(tmp, 1, size, f) == size);
  }

  if (success) {
    buffer->assign(tmp, size);
  }

  free(tmp);
  fclose(f);
  return success;
}

void GenerateKeepList(std::mt19937& rand_gen, std::vector<unsigned short> *keep_list) {
  int list_length;

  switch (rand_gen() & 3) {
    case 0: list_length = 1 + (rand_gen() % 10); break;
    case 1: list_length = 1 + (rand_gen() % 100); break;
    case 2: list_length = 1 + (rand_gen() % 1000); break;
    case 3: list_length = 1 + (rand_gen() % 10000); break;
  }

  for (int i = 0; i < list_length; i++) {
    keep_list->push_back(rand_gen() % (list_length * 2));
  }
}

unsigned long FontSubCreate(const std::string& input_data, std::mt19937& rand_gen, unsigned short usFormat, std::string *output_data) {
  unsigned char *puchFontPackageBuffer = NULL;
  unsigned long ulFontPackageBufferSize = 0;
  unsigned long ulBytesWritten = 0;

  unsigned short usFlag = TTFCFP_FLAGS_SUBSET;
  unsigned short usTTCIndex = 0;

  if (rand_gen() & 1) { usFlag |= TTFCFP_FLAGS_COMPRESS; }
  if (rand_gen() & 1) { usFlag |= TTFCFP_FLAGS_GLYPHLIST; }
  if (input_data.size() >= 12 && !memcmp(input_data.data(), "ttcf", 4)) {
    int font_count = input_data.data()[11];
    if (font_count == 0) {
      font_count = 1;
    }

    usFlag |= TTFCFP_FLAGS_TTC;
    usTTCIndex = rand_gen() % font_count;
  }

  unsigned short usSubsetPlatform = TTFCFP_UNICODE_PLATFORMID;
  unsigned short usSubsetEncoding = TTFCFP_DONT_CARE;

  switch (rand_gen() & 3) {
    case 0:
      usSubsetPlatform = TTFCFP_UNICODE_PLATFORMID;
      break;

    case 1:
      usSubsetPlatform = TTFCFP_APPLE_PLATFORMID;
      if (rand_gen() & 1) {
        usSubsetEncoding = TTFCFP_STD_MAC_CHAR_SET;
      }
      break;

    case 2:
      usSubsetPlatform = TTFCFP_ISO_PLATFORMID;
      break;

    case 3:
      usSubsetPlatform = TTFCFP_MS_PLATFORMID;

      switch (rand_gen() & 3) {
        case 0:
          usSubsetEncoding = TTFCFP_SYMBOL_CHAR_SET;
          break;

        case 1:
          usSubsetEncoding = TTFCFP_UNICODE_CHAR_SET;
          break;
      }
      break;
  }

  std::vector<unsigned short> keep_list;
  GenerateKeepList(rand_gen, &keep_list);

  unsigned long ret = CreateFontPackage(
    (unsigned char *)input_data.data(),
    input_data.size(),
    &puchFontPackageBuffer,
    &ulFontPackageBufferSize,
    &ulBytesWritten,
    usFlag,
    usTTCIndex,
    usFormat,
    /*usSubsetLanguage=*/0,
    usSubsetPlatform,
    usSubsetEncoding,
    &keep_list[0],
    keep_list.size(),
    CfpAllocproc,
    CfpReallocproc,
    CfpFreeproc,
    NULL);

  if (ret == 0) {
    LOG(("[+] CreateFontPackage returned buffer %p, buffer size 0x%lx, bytes written 0x%lx\n",
         puchFontPackageBuffer, ulFontPackageBufferSize, ulBytesWritten));

    output_data->assign((char *)puchFontPackageBuffer, ulBytesWritten);
    CfpFreeproc(puchFontPackageBuffer);
  } else {
    output_data->clear();
  }

  return ret;
}

unsigned long FontSubMerge1(const std::string& input_data, unsigned short usMode, std::string *output_data) {
  unsigned char *puchDestBuffer = NULL;
  unsigned long ulDestBufferSize = 0;
  unsigned long ulBytesWritten = 0;

  unsigned long ret = MergeFontPackage(
    /*puchMergeFontBuffer=*/NULL,
    /*ulMergeFontBufferSize=*/0,
    /*puchFontPackageBuffer=*/(const unsigned char *)input_data.data(),
    /*ulFontPackageBufferSize=*/input_data.size(),
    /*ppuchDestBuffer=*/&puchDestBuffer,
    /*pulDestBufferSize=*/&ulDestBufferSize,
    /*pulBytesWritten=*/&ulBytesWritten,
    usMode,
    CfpAllocproc,
    CfpReallocproc,
    CfpFreeproc,
    NULL);

  if (ret == 0) {
    LOG(("[+] MergeFontPackage returned buffer %p, buffer size 0x%lx, bytes written 0x%lx\n",
         puchDestBuffer, ulDestBufferSize, ulBytesWritten));

    output_data->assign((char *)puchDestBuffer, ulBytesWritten);
    CfpFreeproc(puchDestBuffer);
  } else {
    output_data->clear();
  }

  return ret;
}

unsigned long FontSubMerge2(const std::string& merge_font, const std::string& font_package, std::string *output_data) {
  unsigned char *puchDestBuffer = NULL;
  unsigned long ulDestBufferSize = 0;
  unsigned long ulBytesWritten = 0;

  unsigned long ret = MergeFontPackage(
    /*puchMergeFontBuffer=*/(const unsigned char *)merge_font.data(),
    /*ulMergeFontBufferSize=*/merge_font.size(),
    /*puchFontPackageBuffer=*/(const unsigned char *)font_package.data(),
    /*ulFontPackageBufferSize=*/font_package.size(),
    /*ppuchDestBuffer=*/&puchDestBuffer,
    /*pulDestBufferSize=*/&ulDestBufferSize,
    /*pulBytesWritten=*/&ulBytesWritten,
    /*usMode=*/TTFMFP_DELTA,
    CfpAllocproc,
    CfpReallocproc,
    CfpFreeproc,
    NULL);

  if (ret == 0) {
    LOG(("[+] MergeFontPackage returned buffer %p, buffer size 0x%lx, bytes written 0x%lx\n",
         puchDestBuffer, ulDestBufferSize, ulBytesWritten));

    output_data->assign((char *)puchDestBuffer, ulBytesWritten);
    CfpFreeproc(puchDestBuffer);
  } else {
    output_data->clear();
  }

  return ret;
}

void ProcessSample(const std::string& input_data) {
  std::seed_seq seed(input_data.begin(), input_data.end());
  std::mt19937 rand_gen(seed);

  if (rand_gen() & 1) /* TTFCFP_SUBSET case */ {
    std::string font_package;
    unsigned long ret = FontSubCreate(input_data, rand_gen, TTFCFP_SUBSET, &font_package);

    printf("[+] CreateFontPackage([ %zu bytes ], TTFCFP_SUBSET) ---> %lu (%zu bytes)\n",
           input_data.size(), ret, font_package.size());

    if (ret == 0) {
      std::string working_font;
      ret = FontSubMerge1(font_package, TTFMFP_SUBSET, &working_font);

      printf("[+] MergeFontPackage(NULL, [ %zu bytes ], TTFMFP_SUBSET) ---> %lu (%zu bytes)\n",
             font_package.size(), ret, working_font.size());
    }
  } else /* TTFCFP_SUBSET1 / TTFCFP_DELTA case */ {
    std::string font_package;
    unsigned long ret = FontSubCreate(input_data, rand_gen, TTFCFP_SUBSET1, &font_package);

    printf("[+] CreateFontPackage([ %zu bytes ], TTFCFP_SUBSET1) ---> %lu (%zu bytes)\n",
           input_data.size(), ret, font_package.size());

    if (ret == 0) {
      std::string working_font;
      ret = FontSubMerge1(font_package, TTFMFP_SUBSET1, &working_font);

      printf("[+] MergeFontPackage(NULL, [ %zu bytes ], TTFMFP_SUBSET1) ---> %lu (%zu bytes)\n",
             font_package.size(), ret, working_font.size());

      const int merges = 1 + (rand_gen() % 5);
      for (int i = 0; i < merges; i++) {
        ret = FontSubCreate(input_data, rand_gen, TTFCFP_DELTA, &font_package);
        printf("[+] CreateFontPackage([ %zu bytes ], TTFCFP_DELTA) ---> %lu (%zu bytes)\n",
               input_data.size(), ret, font_package.size());

        if (ret == 0) {
          std::string new_working_font;
          ret = FontSubMerge2(working_font, font_package, &new_working_font);

          printf("[+] MergeFontPackage([ %zu bytes ], [ %zu bytes ], TTFMFP_DELTA) ---> %lu (%zu bytes)\n",
                 working_font.size(), font_package.size(), ret, new_working_font.size());

          if (ret == 0) {
            working_font = new_working_font;
          }
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <path to font file>\n", argv[0]);
    return 1;
  }

  const char *input_font_path = argv[1];

  std::string font_data;
  if (!ReadFileToString(input_font_path, &font_data)) {
    printf("[-] Unable to read the %s input file\n", input_font_path);
    return 1;
  }

  ProcessSample(font_data);

  return 0;
}

