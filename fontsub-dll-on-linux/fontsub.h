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

#ifndef FONTSUB_H_
#define FONTSUB_H_

// Standard FontSub constants from the Windows SDK (fontsub.h).

/* for usSubsetFormat */
#define TTFCFP_SUBSET 0	  /* Straight Subset Font - Backward compatibility */
#define TTFCFP_SUBSET1 1	  /* Subset font with full TTO and Kern tables. For later merge */
#define TTFCFP_DELTA 2	  /* Delta font, for merge with a subset1 font */

/* for usSubsetPlatform */
#define TTFCFP_UNICODE_PLATFORMID 0
#define TTFCFP_APPLE_PLATFORMID   1
#define TTFCFP_ISO_PLATFORMID     2
#define TTFCFP_MS_PLATFORMID      3

/* for usSubsetEncoding */
#define TTFCFP_STD_MAC_CHAR_SET  0	/* goes with TTFSUB_APPLE_PLATFORMID */
#define TTFCFP_SYMBOL_CHAR_SET  0	/* goes with TTFSUB_MS_PLATFORMID */
#define TTFCFP_UNICODE_CHAR_SET  1	/* goes with TTFSUB_MS_PLATFORMID */
#define TTFCFP_DONT_CARE  0xFFFF

/* for usSubsetLanguage */
#define TTFCFP_LANG_KEEP_ALL 0

/* for usFlags */
#define TTFCFP_FLAGS_SUBSET 0x0001	/* if bit off, don't subset */
#define TTFCFP_FLAGS_COMPRESS 0x0002  /* if bit off, don't compress */
#define TTFCFP_FLAGS_TTC 0x0004  /* if bit off, its a TTF */
#define TTFCFP_FLAGS_GLYPHLIST 0x0008 /* if bit off, list is characters */

/* for usModes */
#define TTFMFP_SUBSET 0   /* copy a Straight Subset Font package to Dest buffer */
#define TTFMFP_SUBSET1 1  /* Expand a format 1 font into a format 3 font */
#define TTFMFP_DELTA 2	   /* Merge a format 2 with a format 3 font */

#endif  // FONTSUB_H_

