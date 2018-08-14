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

#ifndef SFNT_FONT_H_
#define SFNT_FONT_H_

#include <stdint.h>

#include <string>
#include <vector>

#pragma pack(push, 1)
struct SfntHeader {
  uint32_t version;
  uint16_t num_tables;
  uint16_t search_range;
  uint16_t entry_selector;
  uint16_t range_shift;
};

struct SfntTableHeader {
  uint32_t tag;
  uint32_t checksum;
  uint32_t offset;
  uint32_t length;
};
#pragma pack(pop)

struct SfntTable {
  uint32_t tag;
  std::string data;
};

class SfntFont {
 public:
  SfntFont() : sfnt_version_(0) { }
  SfntFont(const SfntFont& font);
  ~SfntFont() { }

  bool LoadFromFile(const char *file_path);
  bool SaveToFile(const char *file_path);
  bool SaveToString(std::string *buffer);

  uint32_t sfnt_version_;
  std::vector<SfntTable> sfnt_tables_;

 private:
  uint32_t CalculateChecksum(const std::string& data);
};

#endif  // SFNT_FONT_H_

