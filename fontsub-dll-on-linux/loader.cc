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

#if __x86_64__
#error The loader only works in x86 mode.
#endif  // __x86_64__

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <random>
#include <vector>

#include <dlfcn.h>
#include <sys/mman.h>

#include <parser-library/parse.h>

#include "fontsub.h"

using namespace std;
using namespace peparse;

#ifdef DEBUG
# define LOG(x) printf x;
#else
# define LOG(x)
#endif  // DEBUG

namespace globals {

// See https://docs.microsoft.com/en-us/windows/win32/api/fontsub/ for the
// documentation of the prototypes below.

typedef void *(*CFP_ALLOCPROC)(size_t Arg1);
typedef void *(*CFP_REALLOCPROC)(void *, size_t Arg1);
typedef void (*CFP_FREEPROC)(void *);

typedef unsigned long (*fnCreateFontPackage)(
  const unsigned char  *puchSrcBuffer,
  const unsigned long  ulSrcBufferSize,
  unsigned char        **ppuchFontPackageBuffer,
  unsigned long        *pulFontPackageBufferSize,
  unsigned long        *pulBytesWritten,
  const unsigned short usFlag,
  const unsigned short usTTCIndex,
  const unsigned short usSubsetFormat,
  const unsigned short usSubsetLanguage,
  const unsigned short usSubsetPlatform,
  const unsigned short usSubsetEncoding,
  const unsigned short *pusSubsetKeepList,
  const unsigned short usSubsetListCount,
  CFP_ALLOCPROC        lpfnAllocate,
  CFP_REALLOCPROC      lpfnReAllocate,
  CFP_FREEPROC         lpfnFree,
  void                 *lpvReserved
);

typedef unsigned long (*fnMergeFontPackage)(
  const unsigned char  *puchMergeFontBuffer,
  const unsigned long  ulMergeFontBufferSize,
  const unsigned char  *puchFontPackageBuffer,
  const unsigned long  ulFontPackageBufferSize,
  unsigned char        **ppuchDestBuffer,
  unsigned long        *pulDestBufferSize,
  unsigned long        *pulBytesWritten,
  const unsigned short usMode,
  CFP_ALLOCPROC        lpfnAllocate,
  CFP_REALLOCPROC      lpfnReAllocate,
  CFP_FREEPROC         lpfnFree,
  void                 *lpvReserved
);

fnCreateFontPackage pfnCreateFontPackage;
fnMergeFontPackage pfnMergeFontPackage;

// The delta between the actual address the library is being loaded to in
// relation to the suggested ImageBase from the PE headers.
size_t base_address_delta;

}  // namespace globals

static void *CfpAllocproc(size_t Arg1) {
  void *ret = malloc(Arg1);
  LOG(("[A] malloc(0x%zx) ---> %p\n", Arg1, ret));
  return ret;
}

static void CfpFreeproc(void *Arg1) {
  LOG(("[A] free(%p)\n", Arg1));
  return free(Arg1);
}

static void *CfpReallocproc(void *Arg1, size_t Arg2) {
  void *ret = realloc(Arg1, Arg2);
  LOG(("[A] realloc(%p, 0x%zx) ---> %p\n", Arg1, Arg2, ret));
  return ret;
}

static int MapSection(void *cbd, VA base_address, string& name,
                      image_section_header hdr, bounded_buffer *data) {

  // Apply potential relocation.
  base_address += globals::base_address_delta;

  LOG(("[*] Attempting to map section %-8s base %zx, size %u\n",
       name.c_str(), (size_t)base_address, data->bufLen));

  // Create an initial writable mapping in memory to copy the section's data.
  void *mmapped_addr = mmap((void *)base_address,
                            hdr.Misc.VirtualSize,
                            PROT_WRITE,
                            MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE,
                            -1,
                            0);

  if (mmapped_addr == (void *)-1) {
    perror("MapSection");
    exit(1);
  }

  // Copy the physical data there.
  memcpy(mmapped_addr, data->buf, data->bufLen);

  LOG(("[+] SUCCESS!\n"));
  return 0;
}

static int SetSectionAccessRights(void *cbd, VA base_address, string& name,
                                  image_section_header hdr, bounded_buffer *data) {

  // Apply potential relocation.
  base_address += globals::base_address_delta;

  LOG(("[*] Setting desired access rights for section %-8s\n", name.c_str()));

  // Change the protection to the desired one specified in the section's header.
  int prot = PROT_READ;
  if ((hdr.Characteristics & IMAGE_SCN_CNT_CODE) ||
      (hdr.Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
    prot |= PROT_EXEC;
  }
  if (hdr.Characteristics & IMAGE_SCN_MEM_WRITE) {
    prot |= PROT_WRITE;
  }

  if (mprotect((void *)base_address, hdr.Misc.VirtualSize, prot) == -1) {
    perror("SetSectionAccessRights");
    exit(1);
  }

  LOG(("[+] SUCCESS!\n"));
  return 0;
}

static int ResolveImport(void *cbd, VA import_addr, const string& module_name,
                         const string& function_name) {

  // Apply potential relocation.
  import_addr += globals::base_address_delta;

  LOG(("[*] Resolving the %s!%s import.\n", module_name.c_str(), function_name.c_str()));

  // Initialize the libc handle, if it hasn't been yet.
  static void *libc_handle = NULL;
  if (libc_handle == NULL) {
    libc_handle = dlopen("libc.so.6", RTLD_NOW);
    if (libc_handle == NULL) {
      fprintf(stderr, "ResolveImport: Unable to get the base address of libc.\n");
      exit(1);
    }
    
    LOG(("[+] Resolved the libc handle at %p\n", libc_handle));
  }

  // Insert hooks for target-specific functions.
  void *hook_func_addr = NULL;
  if (function_name == "malloc") {
    hook_func_addr = (void *)CfpAllocproc;
  } else if (function_name == "realloc") {
    hook_func_addr = (void *)CfpReallocproc;
  } else if (function_name == "free") {
    hook_func_addr = (void *)CfpFreeproc;
  }

  if (hook_func_addr != NULL) {
    memcpy((void *)import_addr, &hook_func_addr, sizeof(void *));
    return 0;
  }

  // In the rest of the function, we only do MSVCRT => libc remapping
  // Other imports are filled with a 0xcccccccc marker.
  if (module_name != "MSVCRT.DLL") {
    memcpy((void *)import_addr, "\xcc\xcc\xcc\xcc", 4);
    return 0;
  }

  // Locate the corresponding libc function and insert its address in IAT.
  void *func_addr = dlsym(libc_handle, function_name.c_str());
  if (func_addr != NULL) {
    LOG(("[+] Function %s found in libc at address %p.\n", function_name.c_str(), func_addr));

    memcpy((void *)import_addr, &func_addr, sizeof(void *));
  } else {
    LOG(("[!] Function %s NOT FOUND in libc.\n", function_name.c_str()));

    // The marker for unmatched MSVCRT functions is 0xdddddddd.
    memcpy((void *)import_addr, "\xdd\xdd\xdd\xdd", 4);
  }

  return 0;
}

int ResolveRelocation(void *cbd, VA reloc_addr, reloc_type type) {
  if (type == HIGHLOW) {
    uint32_t *address = (uint32_t *)(reloc_addr + globals::base_address_delta);
    *address += globals::base_address_delta;
  } else if (type != ABSOLUTE) {
    fprintf(stderr, "[-] Unknown relocation type: %d\n", type);
  }

  return 0;
}

static int FindExports(void *cbd, VA func_addr, string& module_name, string& function_name) {
  if (function_name == "CreateFontPackage") {
    globals::pfnCreateFontPackage =
        (globals::fnCreateFontPackage)(func_addr + globals::base_address_delta);
  } else if (function_name == "MergeFontPackage") {
    globals::pfnMergeFontPackage =
        (globals::fnMergeFontPackage)(func_addr + globals::base_address_delta);
  }
  
  return 0;
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

static void GenerateKeepList(std::mt19937& rand_gen,
                             std::vector<unsigned short> *keep_list) {
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

unsigned long FontSubCreate(const std::string& input_data, std::mt19937& rand_gen,
                            unsigned short usFormat, std::string *output_data) {
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
      usSubsetPlatform  = TTFCFP_UNICODE_PLATFORMID;
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

  unsigned long ret = globals::pfnCreateFontPackage(
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
    output_data->assign((char *)puchFontPackageBuffer, ulBytesWritten);
    CfpFreeproc(puchFontPackageBuffer);
  } else {
    output_data->clear();
  }

  return ret;
}

unsigned long FontSubMerge1(const std::string& input_data, unsigned short usMode,
                            std::string *output_data) {
  unsigned char *puchDestBuffer = NULL;
  unsigned long ulDestBufferSize = 0;
  unsigned long ulBytesWritten = 0;

  unsigned long ret = globals::pfnMergeFontPackage(
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
    output_data->assign((char *)puchDestBuffer, ulBytesWritten);
    CfpFreeproc(puchDestBuffer);
  } else {
    output_data->clear();
  }

  return ret;
}

unsigned long FontSubMerge2(const std::string& merge_font, const std::string& font_package,
                            std::string *output_data) {
  unsigned char *puchDestBuffer = NULL;
  unsigned long ulDestBufferSize = 0;
  unsigned long ulBytesWritten = 0;

  unsigned long ret = globals::pfnMergeFontPackage(
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
  if (argc < 3 || argc > 4) {
    printf("Usage: %s /path/to/fontsub.dll /path/to/font/file [image base]\n", argv[0]);
    return 1;
  }

  const char *fontsub_dll_path = argv[1];
  const char *input_font_path = argv[2];

  // Parse the input PE file.
  parsed_pe *pe = ParsePEFromFile(fontsub_dll_path);
  if (pe == NULL) {
    fprintf(stderr, "[-] Unable to parse the %s file.\n", fontsub_dll_path);
    return 1;
  }
  LOG(("[+] Provided fontsub.dll file successfully parsed.\n"));

  // In case of an alternate image base, save information about the address delta.
  if (argc == 4) {
    size_t new_base_address = 0;
    if (sscanf(argv[3], "%zx", &new_base_address) != 1 ||
        (new_base_address & 0xffff) != 0) {
      fprintf(stderr, "[-] Invalid base address %s\n", argv[3]);
      DestructParsedPE(pe);
      return 1;
    }

    size_t orig_base_address = 0;
    if (pe->peHeader.nt.OptionalMagic == NT_OPTIONAL_32_MAGIC) {
      orig_base_address = pe->peHeader.nt.OptionalHeader.ImageBase;
    } else {
      orig_base_address = pe->peHeader.nt.OptionalHeader64.ImageBase;
    }

    globals::base_address_delta = new_base_address - orig_base_address;
  }

  // Map sections to their respective addresses.
  IterSec(pe, MapSection, NULL);

  // Resolve imports (redirect MSVCRT to libc, install stubs etc.) in IAT.
  IterImpVAString(pe, ResolveImport, NULL);

  // If we're loading the library to non-default image base, resolve relocations.
  if (globals::base_address_delta != 0) {
    IterRelocs(pe, ResolveRelocation, NULL);
  }

  // Set the final access rights for the section memory regions.
  IterSec(pe, SetSectionAccessRights, NULL);

  // Find the fontsub!CreateFontPackage and fontsub!MergeFontPackage exported functions.
  IterExpVA(pe, FindExports, NULL);
  
  if (globals::pfnCreateFontPackage == NULL || globals::pfnMergeFontPackage == NULL) {
    fprintf(stderr, "[-] Unable to locate the required exported functions.\n");
    DestructParsedPE(pe);
    return 1;
  }
  LOG(("[+] Located CreateFontPackage at %p, MergeFontPackage at %p.\n",
       globals::pfnCreateFontPackage, globals::pfnMergeFontPackage));

  // Read the input font file.
  std::string font_data;
  if (!ReadFileToString(input_font_path, &font_data)) {
    fprintf(stderr, "[-] Unable to read the %s input file\n", input_font_path);
    DestructParsedPE(pe);
    return 1;
  }

  // Process the input data using the loaded DLL file.
  ProcessSample(font_data);
 
  // Destroy the PE object, but don't manually unmap the image from memory.
  // Instead, rely on the OS for the cleanup.
  DestructParsedPE(pe);

  return 0;
}

