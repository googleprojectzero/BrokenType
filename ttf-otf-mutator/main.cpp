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

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif  // defined(_MSC_VER)

#include <map>
#include <string>
#include <vector>

#include "common.h"
#include "sfnt_font.h"
#include "sfnt_mutator.h"

static bool WriteStringToFile(const char *filename, const std::string& data) {
  FILE *f = fopen(filename, "w+b");
  if (f == NULL) {
    return false;
  }

  if (fwrite(data.data(), sizeof(uint8_t), data.size(), f) != data.size()) {
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input font> <output font>\n", argv[0]);
    return 1;
  }

  const char *input_file_path = argv[1];
  const char *output_file_path = argv[2];

  SfntFont font;
  if (!font.LoadFromFile(input_file_path)) {
    fprintf(stderr, "[-] Unable to load the \"%s\" file as a TTF/OTF font.\n", input_file_path);
    return 1;
  }
  
  SfntStrategies strategies;
  InitSfntMutationStrategies(&strategies);
  MutateSfntFile(&strategies, &font);

  std::string output_data;
  if (!font.SaveToString(&output_data)) {
    fprintf(stderr, "[-] Unable to save a SFNT font to a string.\n");
    return 1;
  }

  if (!WriteStringToFile(output_file_path, output_data)) {
    fprintf(stderr, "[-] Unable to save the output font to \"%s\".\n", output_file_path);
    return 1;
  }

  printf("[+] Font successfully mutated and saved in \"%s\".\n", output_file_path);

  return 0;
}
