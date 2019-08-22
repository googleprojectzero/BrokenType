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
from fontTools.ttLib import TTFont

# Configuration constants.
GLYPHS_PER_SEGMENT = 50
SEGMENTS_PER_PAGE = 75
GLYPHS_PER_PAGE = SEGMENTS_PER_PAGE * GLYPHS_PER_SEGMENT
PAGE_OBJECT_IDX_START = 100
PAGE_CONTENTS_IDX_START = 200

def main(argv):
  if len(argv) != 3:
    print "Usage: %s <font file> <output .pdf path>" % argv[0]
    sys.exit(1)

  # Obtain the number of glyphs in the font, and calculate the number of necessary pages.
  font = TTFont(argv[1])
  glyph_count = len(font.getGlyphSet())
  print "Glyphs in font: %d" % glyph_count

  page_count = int(math.ceil(float(glyph_count) / GLYPHS_PER_PAGE))
  print "Generated pages: %d" % page_count

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
  pdf_data += "  /Count %d\n" % page_count
  pdf_data += "  /Kids [ %s ]\n" % " ".join(map(lambda x:" %d 0 R" % (PAGE_OBJECT_IDX_START + x), (i for i in xrange(page_count))))
  pdf_data += ">>\n"
  pdf_data += "endobj\n"
  pdf_data += "\n"

  # Construct the page descriptor objects.
  for i in xrange(page_count):
    pdf_data += "%d 0 obj\n" % (PAGE_OBJECT_IDX_START + i)
    pdf_data += "<<\n"
    pdf_data += "  /Type /Page\n"
    pdf_data += "  /MediaBox [0 0 612 792]\n"
    pdf_data += "  /Contents %d 0 R\n" % (PAGE_CONTENTS_IDX_START + i)
    pdf_data += "  /Parent 2 0 R\n"
    pdf_data += "  /Resources <<\n"
    pdf_data += "    /Font <<\n"
    pdf_data += "      /CustomFont <<\n"
    pdf_data += "        /Type /Font\n"
    pdf_data += "        /Subtype /Type0\n"
    pdf_data += "        /BaseFont /TestFont\n"
    pdf_data += "        /Encoding /Identity-H\n"
    pdf_data += "        /DescendantFonts [4 0 R]\n"
    pdf_data += "      >>\n"
    pdf_data += "    >>\n"
    pdf_data += "  >>\n"
    pdf_data += ">>\n"
    pdf_data += "endobj\n"

  # Construct the font body object.
  with open(argv[1], "rb") as f:
      font_data = f.read()

  pdf_data += "3 0 obj\n"
  pdf_data += "<</Subtype /OpenType/Length %d>>stream\n" % len(font_data)
  pdf_data += font_data
  pdf_data += "\nendstream\n"
  pdf_data += "endobj\n"
  
  # Construct the page contents objects.
  for i in xrange(page_count):
    pdf_data += "%d 0 obj\n" % (PAGE_CONTENTS_IDX_START + i)
    pdf_data += "<< >>\n"
    pdf_data += "stream\n"
    pdf_data += "BT\n"

    for j in xrange(SEGMENTS_PER_PAGE):
      if i * GLYPHS_PER_PAGE + j * GLYPHS_PER_SEGMENT >= glyph_count:
        break

      pdf_data += "  /CustomFont 12 Tf\n"
      if j == 0:
        pdf_data += "  10 775 Td\n"
      else:
        pdf_data += "  0 -10 Td\n"
      pdf_data += "  <%s> Tj\n" % "".join(map(lambda x: struct.pack(">H", x), (x for x in xrange(i * GLYPHS_PER_PAGE + j * GLYPHS_PER_SEGMENT, i * GLYPHS_PER_PAGE + (j + 1) * GLYPHS_PER_SEGMENT)))).encode("hex")

    pdf_data += "ET\n"
    pdf_data += "endstream\n"
    pdf_data += "endobj\n"

  # Construct the descendant font object.
  pdf_data += "4 0 obj\n"
  pdf_data += "<<\n"
  pdf_data += "  /FontDescriptor <<\n"
  pdf_data += "    /Type /FontDescriptor\n"
  pdf_data += "    /FontName /TestFont\n"
  pdf_data += "    /Flags 5\n"
  pdf_data += "    /FontBBox[0 0 10 10]\n"
  pdf_data += "    /FontFile3 3 0 R\n"
  pdf_data += "  >>\n"
  pdf_data += "  /Type /Font\n"
  pdf_data += "  /Subtype /OpenType\n"
  pdf_data += "  /BaseFont /TestFont\n"
  pdf_data += "  /Widths [1000]\n"
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
