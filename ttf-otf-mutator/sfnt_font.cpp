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

#include "sfnt_font.h"

#include <cstdint>

#include "common.h"

SfntFont::SfntFont(const SfntFont& font) {
  sfnt_version_ = font.sfnt_version_;
  sfnt_tables_ = font.sfnt_tables_;
}

bool SfntFont::LoadFromFile(const char *file_path) {
  // Open the file.
  FILE *f = fopen(file_path, "rb");
  if (f == NULL) {
    goto error;
  }

  // Read the file header, but only save the SFNT version; other elements will
  // be recalculated while writing the output file.
  SfntHeader hdr;
  uint16_t num_tables;
  if (fread(&hdr, sizeof(SfntHeader), 1, f) != 1) {
    goto error;
  }

  sfnt_version_ = hdr.version;

  num_tables = SWAP16(hdr.num_tables);
  sfnt_tables_.resize(num_tables);

  // Read the table headers.
  for (uint32_t i = 0; i < num_tables; i++) {
    if (fseek(f, sizeof(SfntHeader) + i * sizeof(SfntTableHeader), SEEK_SET) != 0) {
      goto error;
    }

    SfntTableHeader table_hdr;
    if (fread(&table_hdr, sizeof(SfntTableHeader), 1, f) != 1) {
      goto error;
    }

    uint32_t table_offset = SWAP32(table_hdr.offset);
    uint32_t table_length = SWAP32(table_hdr.length);

    void *buffer = malloc(table_length);
    if (buffer == NULL) {
      goto error;
    }

    if (fseek(f, table_offset, SEEK_SET) != 0) {
      free(buffer);
      goto error;
    }

    if (fread(buffer, sizeof(uint8_t), table_length, f) != table_length) {
      free(buffer);
      goto error;
    }

    sfnt_tables_[i].tag = SWAP32(table_hdr.tag);
    sfnt_tables_[i].data.assign((char *)buffer, table_length);

    free(buffer);
  }

  // Close the file.
  fclose(f);
  f = NULL;

  return true;

error:
  if (f != NULL) {
    fclose(f);
  }
  sfnt_version_ = 0;
  sfnt_tables_.clear();
  return false;
}

bool SfntFont::SaveToFile(const char *file_path) {
  // Save the font to a string.
  std::string font_contents;
  if (!SaveToString(&font_contents)) {
    return false;
  }

  // Open the file.
  FILE *f = fopen(file_path, "w+b");
  if (f == NULL) {
    return false;
  }

  if (fwrite(font_contents.data(), sizeof(uint8_t), font_contents.size(), f) != font_contents.size()) {
    fclose(f);
    return false;
  }

  // Close the file.
  fclose(f);

  return true;
}

bool SfntFont::SaveToString(std::string *buffer) {
  buffer->clear();

  // Craft and write the file header.
  SfntHeader hdr;
  hdr.version = sfnt_version_;
  hdr.num_tables = SWAP16(sfnt_tables_.size());
  hdr.search_range = 1;
  hdr.entry_selector = 0;
  while (2 * hdr.search_range <= sfnt_tables_.size()) {
    hdr.search_range *= 2;
    hdr.entry_selector++;
  }
  hdr.search_range = SWAP16(16 * hdr.search_range);
  hdr.entry_selector = SWAP16(hdr.entry_selector);
  hdr.range_shift = SWAP16((sfnt_tables_.size() * 16) - SWAP16(hdr.search_range));

  buffer->append((char *)&hdr, sizeof(SfntHeader));

  // Craft and write the table headers.
  uint32_t curr_offset = sizeof(SfntHeader) + sfnt_tables_.size() * sizeof(SfntTableHeader);
  std::vector<uint32_t> table_offsets;
  for (unsigned int i = 0; i < sfnt_tables_.size(); i++) {
    SfntTableHeader table_hdr;
    table_hdr.tag = SWAP32(sfnt_tables_[i].tag);
    table_hdr.checksum = SWAP32(CalculateChecksum(sfnt_tables_[i].data));
    table_hdr.offset = SWAP32(curr_offset);
    table_hdr.length = SWAP32(sfnt_tables_[i].data.size());

    // Account for the padding when calculating table data offset.
    curr_offset += ((sfnt_tables_[i].data.size() + 3) & (-4));

    buffer->append((char *)&table_hdr, sizeof(SfntTableHeader));
  }

  // Write the actual data segments to the file.
  for (unsigned int i = 0; i < sfnt_tables_.size(); i++) {
    buffer->append(sfnt_tables_[i].data.data(), sfnt_tables_[i].data.size());

    // Insert padding bytes if necessary.
    if (sfnt_tables_[i].data.size() & 3) {
      buffer->append(4 - (sfnt_tables_[i].data.size() & 3), '\0');
    }
  }
  return true;
}

uint32_t SfntFont::CalculateChecksum(const std::string& data) {
  uint32_t sum = 0, i;

  for (i = 0; i + 3 < data.size(); i += 4) {
    sum += SWAP32(*(uint32_t *)&data[i]);
  }

  if (i != data.size()) {
    char buf[4] = {0};
    for (uint32_t j = 0; i < data.size(); i++, j++) {
      buf[j] = data[i];
    }
    sum += SWAP32(*(uint32_t *)&buf[0]);
  }

  return sum;
}

