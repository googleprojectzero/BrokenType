# Author: Mateusz Jurczyk (mjurczyk@google.com)
#
# Copyright 2019 Google LLC
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# https://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import math
import os
import struct
import sys
from fontTools.t1Lib import T1Font

# Configuration constants.
GLYPHS_PER_SEGMENT = 50

def main(argv):
  if len(argv) != 3:
    print "Usage: %s <.pfb font file> <output .pdf path>" % argv[0]
    sys.exit(1)

  # Load the number of glyphs in the font.
  font = T1Font(argv[1])
  glyph_count = len(font.getGlyphSet())
  print "Glyphs in font: %d" % glyph_count

  # Craft the PDF header.
  pdf_data = "%PDF-1.1\n"
  pdf_data += "\n"
  pdf_data += "1 0 obj\n"
  pdf_data += "<< /Pages 2 0 R >>\n"
  pdf_data += "endobj\n"
  pdf_data += "\n"
  pdf_data += "2 0 obj\n"
  pdf_data += "<<\n"
  pdf_data += "  /Type /Pages\n"
  pdf_data += "  /Count 1\n"
  pdf_data += "  /Kids [ 3 0 R ]\n"
  pdf_data += ">>\n"
  pdf_data += "endobj\n"
  pdf_data += "\n"
  pdf_data += "3 0 obj\n"
  pdf_data += "<<\n"
  pdf_data += "  /Type /Page\n"
  pdf_data += "  /MediaBox [0 0 612 792]\n"
  pdf_data += "  /Contents 5 0 R\n"
  pdf_data += "  /Parent 2 0 R\n"
  pdf_data += "  /Resources <<\n"
  pdf_data += "    /Font <<\n"
  pdf_data += "      /CustomFont <<\n"
  pdf_data += "        /Type /Font\n"
  pdf_data += "        /Subtype /Type0\n"
  pdf_data += "        /BaseFont /TestFont\n"
  pdf_data += "        /Encoding /Identity-H\n"
  pdf_data += "        /DescendantFonts [6 0 R]\n"
  pdf_data += "      >>\n"
  pdf_data += "    >>\n"
  pdf_data += "  >>\n"
  pdf_data += ">>\n"
  pdf_data += "endobj\n"

  # Construct the font body object.
  with open(argv[1], "rb") as f:
    font_data = f.read()

  pdf_data += "4 0 obj\n"
  pdf_data += "<</Subtype /Type1/Length %d>>stream\n" % len(font_data)
  pdf_data += font_data
  pdf_data += "\nendstream\n"
  pdf_data += "endobj\n"
  
  # Construct the page contents object.
  pdf_data += "5 0 obj\n"
  pdf_data += "<< >>\n"
  pdf_data += "stream\n"
  pdf_data += "BT\n"

  segment_count = int(math.ceil(float(glyph_count) / GLYPHS_PER_SEGMENT))
  for i in xrange(segment_count):
    pdf_data += "  /CustomFont 12 Tf\n"
    if i == 0:
      pdf_data += "  10 780 Td\n"
    else:
      pdf_data += "  0 -10 Td\n"
    pdf_data += "  <%s> Tj\n" % "".join(map(lambda x: struct.pack(">H", x), (x for x in xrange(i * GLYPHS_PER_SEGMENT, (i + 1) * GLYPHS_PER_SEGMENT)))).encode("hex")

  pdf_data += "ET\n"
  pdf_data += "endstream\n"
  pdf_data += "endobj\n"

  # Construct the descendant font object.
  pdf_data += "6 0 obj\n"
  pdf_data += "<<\n"
  pdf_data += "  /FontDescriptor <<\n"
  pdf_data += "    /Type /FontDescriptor\n"
  pdf_data += "    /FontName /TestFont\n"
  pdf_data += "    /Flags 5\n"
  pdf_data += "    /FontBBox[0 0 10 10]\n"
  pdf_data += "    /FontFile3 4 0 R\n"
  pdf_data += "  >>\n"
  pdf_data += "  /Type /Font\n"
  pdf_data += "  /Subtype /Type1\n"
  pdf_data += "  /BaseFont /TestFont\n"
  pdf_data += ">>\n"
  pdf_data += "endobj\n"

  # Construct the trailer.
  pdf_data += "trailer\n"
  pdf_data += "<<\n"
  pdf_data += "  /Root 1 0 R\n"
  pdf_data += ">>\n"

  # Write the output to disk.
  with open(argv[2], "w+b") as f:
    f.write(pdf_data)

if __name__ == "__main__":
  main(sys.argv)
