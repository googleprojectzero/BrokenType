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

#include "mutator.h"

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstring>
#include <vector>

#include "random.h"

namespace globals {

}  // globals

void Mutator::MutateString(const std::vector<MutationStrategy>& strategies, std::string *buffer, unsigned int *changed_bytes) {
  std::vector<MutationStrategy> strats = strategies;

  // Randomly choose the strategies we are going to use for mutation.
  const unsigned int mutations_no = (Random32() % strats.size()) + 1;

  random_shuffle(strats.begin(), strats.end());
  strats.resize(mutations_no);

  // Choose the mutation ratios from available ranges.
  std::vector<double> mutation_ratios;
  for (unsigned int i = 0; i < strats.size(); i++) {
    const double min_ratio = strats[i].min_mutation_ratio;
    const double max_ratio = strats[i].max_mutation_ratio;
    const double ratio = min_ratio + fmod(RandomFloat(), max_ratio - min_ratio + 1e-10);
    mutation_ratios.push_back(ratio);
  }

  // Choose how much each mutation will affect the result.
  std::vector<double> division;
  for (unsigned int i = 0; i < strats.size() - 1; i++) {
    division.push_back(RandomFloat());
  }
  division.push_back(0.0f);
  division.push_back(1.0f);
  sort(division.begin(), division.end());

  for (unsigned int i = 1; i < division.size(); i++) {
    mutation_ratios[i - 1] *= (division[i] - division[i - 1]);
  }

  // Invoke each mutation strategy over the string.
  *changed_bytes = 0;
  for (unsigned int i = 0; i < strats.size(); i++) {
    unsigned int locally_changed_bytes;
    MutateString(strats[i].type, mutation_ratios[i], buffer, &locally_changed_bytes);
    *changed_bytes += locally_changed_bytes;
  }
}

void Mutator::MutateString(MutationType type, double ratio, std::string *buffer, unsigned int *changed_bytes) {
  switch (type) {
    case MUTATION_BITFLIPPING:
      Bitflipping(buffer, ratio, changed_bytes);
      break;

    case MUTATION_BYTEFLIPPING:
      Byteflipping(buffer, ratio, changed_bytes);
      break;

    case MUTATION_CHUNKSPEW:
      Chunkspew(buffer, ratio, changed_bytes);
      break;

    case MUTATION_SPECIAL_INTS:
      SpecialInts(buffer, ratio, changed_bytes);
      break;

    case MUTATION_ADD_SUB_BINARY:
      AddSubBinary(buffer, ratio, changed_bytes);
      break;

    default:
      break;
  }
}

void Mutator::Bitflipping(std::string *buffer, double ratio, unsigned int *changed_bytes) {
  uint32_t bytes_to_mutate = ((double)buffer->size() * ratio) * 8;
  *changed_bytes = bytes_to_mutate;

  while (bytes_to_mutate-- > 0) {
    uint32_t offset = (Random32() % buffer->size());
    const unsigned int bit = (Random32() & 7);
    buffer->at(offset) ^= (1 << bit);
  }
}

void Mutator::Byteflipping(std::string *buffer, double ratio, unsigned int *changed_bytes) {
  uint32_t bytes_to_mutate = ((double)buffer->size() * ratio);
  *changed_bytes = bytes_to_mutate;

  while (bytes_to_mutate-- > 0) {
    const uint32_t offset = (Random32() % buffer->size());
    buffer->at(offset) = (Random32() & 0xff);
  }
}

void Mutator::Chunkspew(std::string *buffer, double ratio, unsigned int *changed_bytes) {
  const unsigned int kMaximumChunkSpew = 64;
  const uint32_t bytes_to_mutate = ((double)buffer->size() * ratio);
  uint32_t mutated_bytes = 0;

  uint8_t chunk_buffer[kMaximumChunkSpew];
  while (mutated_bytes < bytes_to_mutate) {
    const uint32_t mutation_size = (Random32() % MIN3(kMaximumChunkSpew, buffer->size() / 2, bytes_to_mutate - mutated_bytes + 1));
    const uint32_t source_offset = (Random32() % (buffer->size() - mutation_size));
    const uint32_t dest_offset = (Random32() % (buffer->size() - mutation_size));

    memcpy(chunk_buffer, &buffer->data()[source_offset], mutation_size);
    buffer->replace(dest_offset, mutation_size, reinterpret_cast<const char *>(chunk_buffer), mutation_size);
    mutated_bytes += mutation_size;
  }

  *changed_bytes = mutated_bytes;
}

void Mutator::SpecialInts(std::string *buffer, double ratio, unsigned int *changed_bytes) {
  const struct {
    size_t size;
    const char *data;
  } kSpecialBinaryInts[] = {
    {1, "\x00"}, {1, "\x7f"}, {1, "\x80"}, {1, "\xff"},
    {2, "\x00\x00"}, {2, "\x7f\xff"}, {2, "\xff\xff"}, {2, "\x80\x00"},
    {2, "\x40\x00"}, {2, "\xc0\x00"},
    {4, "\x00\x00\x00\x00"}, {4, "\x7f\xff\xff\xff"}, {4, "\x80\x00\x00\x00"},
    {4, "\x40\x00\x00\x00"}, {4, "\xc0\x00\x00\x00"}, {4, "\xff\xff\xff\xff"}
  };
  const uint32_t bytes_to_mutate = ((double)buffer->size() * ratio);
  uint32_t mutated_bytes = 0;

  while (mutated_bytes < bytes_to_mutate) {
    const unsigned int int_idx = (Random32() % (sizeof(kSpecialBinaryInts) / sizeof(kSpecialBinaryInts[0])));
    const unsigned int int_size = kSpecialBinaryInts[int_idx].size;
    uint32_t offset = (Random32() % (buffer->size() - int_size + 1));
    buffer->replace(offset, int_size, kSpecialBinaryInts[int_idx].data, int_size);
    mutated_bytes += int_size;
  }

  *changed_bytes = mutated_bytes;
}

void Mutator::AddSubBinary(std::string *buffer, double ratio, unsigned int *changed_bytes) {
  const uint32_t bytes_to_mutate = ((double)buffer->size() * ratio);
  uint32_t mutated_bytes = 0;

  while (mutated_bytes < bytes_to_mutate) {
    const uint32_t spec = Random32();
    const bool big_endian = ((spec & 1) == 1);
    const bool addition = ((spec & 2) == 2);
    unsigned int unit_width;
    uint32_t max_value;

    // Decide on the unit width.
    switch ((spec >> 2) % 3) {
      case 0:  // Byte arithmetic.
        unit_width = 1;
        max_value = UCHAR_MAX;
        break;
      case 1:   // Word arithmetic.
        unit_width = 2;
        max_value = USHRT_MAX;
        break;
      case 2:  // Dword arithmetic.
        unit_width = 4;
        max_value = UINT_MAX;
        break;
    }

    // Choose offset.
    if (buffer->size() < unit_width) {
      continue;
    }
    const uint32_t offset = (Random32() % (buffer->size() - unit_width + 1));

    // Read the value.
    uint32_t value = 0;
    if (big_endian) {
      for (unsigned int i = 0; i < unit_width; ++i) {
        value |= (buffer->at(offset + i) << (8 * ((unit_width - 1) - i)));
      }
    } else {
      for (unsigned int i = 0; i < unit_width; ++i) {
        value |= (buffer->at(offset + i) << (8 * i));
      }
    }

    // Choose an operand.
    uint32_t op;
    switch (unit_width) {
      case 1:
        op = 1 + (Random32() % UCHAR_MAX);
        break;
      case 2:
      case 4:
        if (value > UCHAR_MAX && (Random32() & 1) == 1) {
          op = 1 + (Random32() % USHRT_MAX);
        } else {
          op = 1 + (Random32() % UCHAR_MAX);
        }
        break;
    }

    // Perform the arithmetic.
    if (addition) {
      if (max_value - value < op) {
        value = max_value;
      } else {
        value += op;
      }
    } else {
      if (value < op) {
        value = 0;
      } else {
        value -= op;
      }
    }

    // Write the data back to buffer and count byte diffs.
    unsigned int byte_delta = 0;
    if (big_endian) {
      for (unsigned int i = 0; i < unit_width; ++i) {
        uint8_t byte = (value >> (8 * ((unit_width - 1) - i)));
        if (buffer->at(offset + i) != byte) {
          buffer->at(offset + i) = byte;
          byte_delta++;
        }
      }
    } else {
      for (unsigned int i = 0; i < unit_width; ++i) {
        uint8_t byte = (value >> (8 * i));
        if (buffer->at(offset + i) != byte) {
          buffer->at(offset + i) = byte;
          byte_delta++;
        }
      }
    }

    // Update the mutation progress made so far.
    mutated_bytes += byte_delta;
  }

  *changed_bytes = mutated_bytes;
}

