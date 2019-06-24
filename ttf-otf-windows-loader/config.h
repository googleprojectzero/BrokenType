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

#ifndef CONFIG_H_
#define CONFIG_H_

// Harness build options (tested font-related functionality)
#define HARNESS_TEST_KERNING_PAIRS  1
#define HARNESS_TEST_DRAWTEXT       1
#define HARNESS_TEST_GLYPH_OUTLINE  1
#define HARNESS_TEST_UNISCRIBE      1

// Number of glyphs displayed at once by a single DrawText() call.
#define DISPLAYED_GLYPHS_COUNT (10)

// Maximum number of alternate glyphs requested through the
// usp10!ScriptGetFontAlternateGlyphs function.
#define MAX_ALTERNATE_GLYPHS (10)

// Maximum number of tags requested through the usp10!ScriptGetFontScriptTags,
// usp10!ScriptGetFontLanguageTags and usp10!ScriptGetFontFeatureTags functions.
#define UNISCRIBE_MAX_TAGS (16)

#endif  // CONFIG_H_
