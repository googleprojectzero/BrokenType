# Author: Mateusz Jurczyk (mjurczyk@google.com)
#
# Copyright 2018 Google LLC
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

import os
import random
import sys
import xml.etree.ElementTree as ET

class TTProgram:
  def __init__(self, twilight_points, contours_in_glyph, points_in_glyph):
    # Specifies the number of points in the Twilight Zone (Z0).
    self._twilight_points = twilight_points

    # Specifies the number of contours making up the glyph.
    self._contours_in_glyph = contours_in_glyph

    # Specifies the number of points existing in the glyph.
    self._points_in_glyph = points_in_glyph

    # Current state of zone pointers.
    self._zp = [1, 1, 1]

  def _Imm_ContourIdx(self, zone_idx):
    """Returns a random contour index for Zone 1 (the Twilight Zone should never contain contours).
    """
    # Once in a while, ignore the constraints and return something possibly invalid.
    if random.randint(0, 1000) == 0:
      if self._contours_in_glyph == 0:
        return self._Imm_SRandom16()
      else:
        return random.randint(0, 2 * self._contours_in_glyph)

    # Otherwise, obey the rules and return a valid contour index.
    assert(self._zp[zone_ptr] != 0)
    assert(self._contours_in_glyph != 0)
    return random.randint(0, self._contours_in_glyph - 1)

  def _Imm_PointIdx(self, zone_ptr):
    """Returns a random point index for the specified zone.
    """

    # Once in a while, ignore the constraints and return something possibly invalid.
    if random.randint(0, 1000) == 0:
      if (self._twilight_points == 0) and (self._points_in_glyph == 0):
        return self._Imm_SRandom16()
      else:
        return random.randint(0, max(self._twilight_points, self._points_in_glyph) - 1)

    # Otherwise, obey the rules and return a valid point index.
    if self._zp[zone_ptr] == 0:
      assert(self._twilight_points != 0)
      point_idx = random.randint(0, self._twilight_points - 1)
    else:
      assert(self._points_in_glyph != 0)
      point_idx = random.randint(0, self._points_in_glyph - 1)

    return point_idx

  def _Imm_SRandom16(self):
    return random.randint(-32768, 32767)

  def _Imm_URandom16(self):
    return random.randint(0, 32767)

  # Stack instructions.
  # Only used as a part of other instructions; not arbitrarily inserted into the generated program.
  def _Instr_PUSH(self, values):
    return ["PUSH[ ]\n" + " ".join(map(lambda x: str(x), values))]

  def _Instr_POP(self):
    return ["POP[ ]"]

  def _Instr_CLEAR(self):
    return ["CLEAR[ ]"]
 
  # Storage Area.
  # Not implemented, as we don't expect that fuzzing these instructions would yield any interesting results.
  def _Instr_RS(self):
    return []

  def _Instr_WS(self):
    return []

  # Control Value Table.
  def _Instr_WCVTP(self):
    return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_SRandom16()]) + # TODO: Change the second Random16() to RandomF26Dot6().
           ["WCVTP[ ]"])

  def _Instr_WCVTF(self):
    return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_SRandom16()]) + # TODO: Change the second Random16() to RandomF26Dot6().
           ["WCVTF[ ]"])

  def _Instr_RCVT(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["RCVT[ ]"] +
            self._Instr_POP())

  # Graphics State.
  def _Instr_SVTCA(self):
    return ["SVTCA[" + str(random.randint(0, 1)) + "]"]

  def _Instr_SPVTCA(self):
    return ["SPVTCA[" + str(random.randint(0, 1)) + "]"]

  def _Instr_SFVTCA(self):
    return ["SFVTCA[" + str(random.randint(0, 1)) + "]"]

  def _Instr_SPVTL(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(1), self._Imm_PointIdx(2)]) +
             ["SPVTL[" + str(random.randint(0, 1)) + "]"])
    except:
      return []

  def _Instr_SFVTL(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(1), self._Imm_PointIdx(2)]) +
             ["SFVTL[" + str(random.randint(0, 1)) + "]"])
    except:
      return []

  def _Instr_SFVTPV(self):
    return ["SFVTPV[ ]"]

  def _Instr_SDPVTL(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(1), self._Imm_PointIdx(2)]) +
             ["SDPVTL[" + str(random.randint(0, 1)) + "]"])
    except:
      return []

  def _Instr_SPVFS(self):
    opt = random.randint(0, 2)
    if opt == 0:
      return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_URandom16()]) +
             ["SPVFS[ ]"])
    elif opt == 1:
      return (self._Instr_PUSH([0x4000, 0]) +
             ["SPVFS[ ]"])
    elif opt == 2:
      return (self._Instr_PUSH([0, 0x4000]) +
             ["SPVFS[ ]"])

  def _Instr_SFVFS(self):
    opt = random.randint(0, 2)
    if opt == 0:
      return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_URandom16()]) +
             ["SFVFS[ ]"])
    elif opt == 1:
      return (self._Instr_PUSH([0x4000, 0]) +
             ["SFVFS[ ]"])
    elif opt == 2:
      return (self._Instr_PUSH([0, 0x4000]) +
             ["SFVFS[ ]"])

  def _Instr_GPV(self):
    # Not implemented, as we don't expect that fuzzing this instruction would yield any results.
    return []

  def _Instr_GFV(self):
    # Not implemented, as we don't expect that fuzzing this instruction would yield any results.
    return []

  def _Instr_SRP0(self, zone_ptr):
    return (self._Instr_PUSH([self._Imm_PointIdx(zone_ptr)]) +
           ["SRP0[ ]"])

  def _Instr_SRP1(self, zone_ptr):
    return (self._Instr_PUSH([self._Imm_PointIdx(zone_ptr)]) +
           ["SRP1[ ]"])

  def _Instr_SRP2(self, zone_ptr):
    return (self._Instr_PUSH([self._Imm_PointIdx(zone_ptr)]) +
           ["SRP2[ ]"])

  def _Instr_SZP0(self):
    zone_ptr = random.randint(0, 1)
    self._zp[0] = zone_ptr
    return (self._Instr_PUSH([zone_ptr]) +
           ["SZP0[ ]"])

  def _Instr_SZP1(self):
    zone_ptr = random.randint(0, 1)
    self._zp[1] = zone_ptr
    return (self._Instr_PUSH([zone_ptr]) +
           ["SZP1[ ]"])

  def _Instr_SZP2(self, zone_ptr = None):
    if zone_ptr == None:
      zone_ptr = random.randint(0, 1)
    self._zp[2] = zone_ptr
    return (self._Instr_PUSH([zone_ptr]) +
           ["SZP2[ ]"])

  def _Instr_SZPS(self):
    zone_ptr = random.randint(0, 1)
    self._zp = [zone_ptr] * 3
    return (self._Instr_PUSH([zone_ptr]) +
           ["SZPS[ ]"])

  def _Instr_RTHG(self):
    return ["RTHG[ ]"]

  def _Instr_RTG(self):
    return ["RTG[ ]"]

  def _Instr_RTDG(self):
    return ["RTDG[ ]"]

  def _Instr_RDTG(self):
    return ["RDTG[ ]"]

  def _Instr_RUTG(self):
    return ["RUTG[ ]"]

  def _Instr_ROFF(self):
    return ["ROFF[ ]"]

  def _Instr_SROUND(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["SROUND[ ]"])

  def _Instr_S45ROUND(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["S45ROUND[ ]"])
  
  def _Instr_SLOOP(self):
    # Not implemented yet, since it is not clear how to do it correctly.
    return []

  def _Instr_SMD(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) + # TODO: Change Random16() to RandomF26Dot6().
           ["SMD[ ]"])

  def _Instr_INSTCTRL(self):
    return (self._Instr_PUSH([self._Imm_SRandom16(), 3]) +
           ["INSTCTRL[ ]"])

  def _Instr_SCANCTRL(self):
    return (self._Instr_PUSH([self._Imm_SRandom16()]) +
           ["SCANCTRL[ ]"])

  def _Instr_SCANTYPE(self):
    return (self._Instr_PUSH([self._Imm_SRandom16()]) +
           ["SCANTYPE[ ]"])

  def _Instr_SCVTCI(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) + # TODO: Change Random16() to RandomF26Dot6().
           ["SCVTCI[ ]"])

  def _Instr_SSWCI(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) + # TODO: Change Random16() to RandomF26Dot6().
           ["SSWCI[ ]"])

  def _Instr_SSW(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) + # TODO: Change Random16() to RandomF26Dot6().
           ["SSW[ ]"])

  def _Instr_FLIPON(self):
    return ["FLIPON[ ]"]
 
  def _Instr_FLIPOFF(self):
    return ["FLIPOFF[ ]"]

  def _Instr_SANGW(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["SANGW[ ]"])

  def _Instr_SDB(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["SDB[ ]"])

  def _Instr_SDS(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["SDS[ ]"])

  # Managing outlines.
  def _Instr_FLIPPT(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(0)]) +
             ["FLIPPT[ ]"])
    except:
      return []

  def _Instr_FLIPRGON(self):
    try:
      x = self._Imm_PointIdx(0)
      y = self._Imm_PointIdx(0)
      if x > y:
        x, y = y, x
      
      return (self._Instr_PUSH([x, y]) +
             ["FLIPRGON[ ]"])
    except:
      return []

  def _Instr_FLIPRGOFF(self):
    try:
      x = self._Imm_PointIdx(0)
      y = self._Imm_PointIdx(0)
      if x > y:
        x, y = y, x
      
      return (self._Instr_PUSH([x, y]) +
             ["FLIPRGOFF[ ]"])
    except:
      return []

  def _Instr_SHP(self):
    try:
      op = []

      flag = random.randint(0, 1)
      if flag == 0:
        op += self._Instr_SRP2(1)
      else:
        op += self._Instr_SRP1(0)

      return (op + self._Instr_PUSH([self._Imm_PointIdx(2)]) +
             ["SHP[" + str(flag) + "]"])
    except:
      return []

  def _Instr_SHC(self):
    try:
      flag = random.randint(0, 1)
      if flag == 0:
        op += self._Instr_SRP2(1)
      else:
        op += self._Instr_SRP1(0)

      return (op + self._Instr_PUSH([self._Imm_ContourIdx(2)]) +
             ["SHC[" + str(flag) + "]"])
    except:
      return []

  def _Instr_SHZ(self):
    try:
      flag = random.randint(0, 1)
      if flag == 0:
        op += self._Instr_SRP2(1)
      else:
        op += self._Instr_SRP1(0)

      return (op + self._Instr_PUSH([random.randint(0, 1)]) +
             ["SHZ[" + str(flag) + "]"])
    except:
      return []

  def _Instr_SHPIX(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(2), self._Imm_URandom16()]) + # TODO: Change Random16() to RandomF26Dot6().
             ["SHPIX[ ]"])
    except:
      return []

  def _Instr_MSIRP(self):
    try:
      return (self._Instr_SRP0(0) +
             self._Instr_PUSH([self._Imm_PointIdx(1), self._Imm_URandom16()]) + # TODO: Change Random16() to RandomF26Dot6().
             ["MSIRP[" + str(random.randint(0, 1)) + "]"])
    except:
      return []

  def _Instr_MDAP(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(0)]) + # TODO: Change Random16() to RandomF26Dot6().
             ["MDAP[" + str(random.randint(0, 1)) + "]"])
    except:
      return []

  def _Instr_MIAP(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(0), self._Imm_URandom16()]) +
             ["MIAP[" + str(random.randint(0, 1)) + "]"])
    except:
      return []

  def _Instr_MDRP(self):
    try:
      return (self._Instr_SRP0(0) +
             self._Instr_PUSH([self._Imm_PointIdx(1)]) +
             ["MDRP[" + str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        "]"])
    except:
      return []

  def _Instr_MIRP(self):
    try:
      return (self._Instr_SRP0(0) +
             self._Instr_PUSH([self._Imm_PointIdx(1), self._Imm_URandom16()]) +
             ["MIRP[" + str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        str(random.randint(0, 1)) +
                        "]"])
    except:
      return []

  def _Instr_ALIGNRP(self):
    try:
      return (self._Instr_SRP0(0) +
             self._Instr_PUSH([self._Imm_PointIdx(1)]) +
             ["ALIGNRP[ ]"])
    except:
      return []

  def _Instr_ISECT(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(2),
                                self._Imm_PointIdx(1), self._Imm_PointIdx(1),
                                self._Imm_PointIdx(0), self._Imm_PointIdx(0)]) +
             ["ISECT[ ]"])
    except:
      return []

  def _Instr_ALIGNPTS(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(0), self._Imm_PointIdx(1)]) +
             ["ALIGNPTS[ ]"])
    except:
      return []

  def _Instr_IP(self):
    try:
      return (self._Instr_SRP1(0) + self._Instr_SRP2(1) +
             self._Instr_PUSH([self._Imm_PointIdx(2)]) +
             ["IP[ ]"])
    except:
      return []

  def _Instr_UTP(self):
    try:
      return (self._Instr_PUSH([self._Imm_PointIdx(0)]) +
             ["UTP[ ]"])
    except:
      return []

  def _Instr_IUP(self):
    if self._points_in_glyph == 0:
      return []
    return ["IUP[" + str(random.randint(0, 1)) + "]"]

  # Managing exceptions.
  def _Instr_DELTAP1(self):
    try:
      return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_PointIdx(0), 1]) +
             ["DELTAP1[ ]"])
    except:
      return []

  def _Instr_DELTAP2(self):
    try:
      return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_PointIdx(0), 1]) +
             ["DELTAP2[ ]"])
    except:
      return []

  def _Instr_DELTAP3(self):
    try:
      return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_PointIdx(0), 1]) +
             ["DELTAP3[ ]"])
    except:
      return []

  def _Instr_DELTAC1(self):
    return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_URandom16(), 1]) +
           ["DELTAC1[ ]"])

  def _Instr_DELTAC2(self):
    return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_URandom16(), 1]) +
           ["DELTAC2[ ]"])

  def _Instr_DELTAC3(self):
    return (self._Instr_PUSH([self._Imm_URandom16(), self._Imm_URandom16(), 1]) +
           ["DELTAC3[ ]"])

  # Compensating for the engine characteristics.
  def _Instr_ROUND(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["ROUND[" + str(random.randint(0, 1)) + str(random.randint(0, 1)) + "]"] +
            self._Instr_POP())

  def _Instr_NROUND(self):
    return (self._Instr_PUSH([self._Imm_URandom16()]) +
           ["NROUND[" + str(random.randint(0, 1)) + str(random.randint(0, 1)) + "]"] +
            self._Instr_POP())

  def GenerateInstruction(self):
    instructions = [
                    # Storage Area.
                    self._Instr_RS,
                    self._Instr_WS,

                    # Control Value Table.
                    self._Instr_WCVTP,
                    self._Instr_WCVTF,
                    self._Instr_RCVT,

                    # Graphics State.
                    self._Instr_SVTCA,
                    self._Instr_SPVTCA,
                    self._Instr_SFVTCA,
                    self._Instr_SPVTL,
                    self._Instr_SFVTL,
                    self._Instr_SFVTPV,
                    self._Instr_SDPVTL,
                    self._Instr_SPVFS,
                    self._Instr_SFVFS,
                    self._Instr_GPV,
                    self._Instr_GFV,
                    #self._Instr_SRP0,  #
                    #self._Instr_SRP1,  # Reference point setting is only performed directly in instructions which use them.
                    #self._Instr_SRP2,  #
                    self._Instr_SZP0,
                    self._Instr_SZP1,
                    self._Instr_SZP2,
                    self._Instr_SZPS,
                    self._Instr_RTHG,
                    self._Instr_RTG,
                    self._Instr_RTDG,
                    self._Instr_RDTG,
                    self._Instr_RUTG,
                    self._Instr_ROFF,
                    self._Instr_SROUND,
                    self._Instr_S45ROUND,
                    self._Instr_SLOOP,
                    self._Instr_SMD,
                    self._Instr_SCANCTRL,
                    self._Instr_SCANTYPE,
                    self._Instr_SCVTCI,
                    self._Instr_SSWCI,
                    self._Instr_SSW,
                    self._Instr_FLIPON,
                    self._Instr_FLIPOFF,
                    self._Instr_SANGW,
                    self._Instr_SDB,
                    self._Instr_SDS,

                    # Managing Outlines.
                    self._Instr_FLIPPT,
                    self._Instr_FLIPRGON,
                    self._Instr_FLIPRGOFF,
                    self._Instr_SHP,
                    self._Instr_SHC,
                    self._Instr_SHZ,
                    self._Instr_SHPIX,
                    self._Instr_MSIRP,
                    self._Instr_MDAP,
                    self._Instr_MIAP,
                    self._Instr_MDRP,
                    self._Instr_MIRP,
                    self._Instr_ALIGNRP,
                    self._Instr_ISECT,
                    self._Instr_ALIGNPTS,
                    self._Instr_IP,
                    self._Instr_UTP,
                    self._Instr_IUP,

                    # Managing Exceptions.
                    self._Instr_DELTAP1,
                    self._Instr_DELTAP2,
                    self._Instr_DELTAP3,
                    self._Instr_DELTAC1,
                    self._Instr_DELTAC2,
                    self._Instr_DELTAC3,

                    # Compensating For The Engine Characteristics.
                    self._Instr_ROUND,
                    self._Instr_NROUND,
                    ]
    
    return random.choice(instructions)()

  def GenerateProgram(self, instructions_limit):
    pgm = self._Instr_FLIPRGON()
    while len(pgm) < instructions_limit:
      pgm += self.GenerateInstruction()

    return "\n".join(pgm)

class TTXParser:

  # Specifies the number of instructions inserted into the "prep" TTF table.
  PREP_INSTRUCTIONS = 10000

  # Specifies the maximum number of instructions per glyph.
  MAX_INSTRUCTIONS_PER_GLYPH = 10000

  def __init__(self, num_instructions):
    # Specifies the total number of TrueType instructions which should be inserted into the font.
    self._num_instructions = num_instructions

    # Specifies the chosen number of twilight points in the font.
    self._twilight_points = 0

    # Specifies the total number of glyphs defined in the font, as extracted from the "maxp" table.
    self._num_glyphs = 0

    # Specifies the number of contours defined in the currently processed glyph.
    self._contours_in_glyph = 0

    # Specifies the number of points defined in the currently processed glyph.
    self._points_in_glyph = 0

  def _Handler_ttFont(self, path, node):
    """Makes sure that the CVT table exists in the font, as it is required for the generated instruction
       stream, and fonttools recalculate maxp.maxStorage based on the size of the table.
    """
    if node.find("cvt") == None:
      ET.SubElement(node, "cvt")

  def _Handler_maxp(self, path, node):
    """Sets the limits specified in the "maxp" TTF table to desired values.
    """
    num_glyphs_node = node.find("numGlyphs")
    assert(num_glyphs_node != None)

    self._num_glyphs = int(num_glyphs_node.attrib["value"])

    max_points = None
    for item in node:
      if item.tag == "maxStorage":
        item.attrib["value"] = str(2 ** 15)
      elif item.tag == "maxStackElements":
        item.attrib["value"] = "8"
      elif item.tag == "maxSizeOfInstructions":
        item.attrib["value"] = str(2 ** 16 - 1)
      elif item.tag == "maxPoints":
        max_points = int(item.attrib["value"])
      elif item.tag == "maxTwilightPoints":
        assert(max_points != None)
        if random.randint(0, 1) == 0:
          self._twilight_points = max(max_points * 2, 100)
        else:
          self._twilight_points = 0
        item.attrib["value"] = str(self._twilight_points)
      elif item.tag == "maxZones":
        item.attrib["value"] = "2"

  def _Handler_cvt(self, path, node):
    """Fills the CVT table with 32768 random values.
    """
    
    # First, iterate through the "cv" elements in order to locate the largest existing index.
    max_index = -1
    for item in node.findall("cv"):
      max_index = max(max_index, int(item.attrib["index"]))
  
    for idx in range(max_index + 1, 32767 + 1):
      ET.SubElement(node, "cv", {"index" : str(idx), "value" : str(random.randint(-32768, 32767))})

  def _Handler_TTGlyph(self, path, node):
    """Resets the number of contours and points in the glyph (because there is a new one), and inserts a child
       <instructions> tag if one is not already present, in order to have TrueType instructions executed for all
       glyphs in the font.
    """
    self._contours_in_glyph = 0
    self._points_in_glyph = 0

    # Only add the instructions for glyphs which have at least one contour.
    if (node.find("contour") != None) and (node.find("instructions") == None):
      instructions_node = ET.SubElement(node, "instructions")
      ET.SubElement(instructions_node, "assembly")

  def _Handler_contour(self, path, node):
    """ Accounts for a new contour in the glyph by incrementing the _contours_in_glyph field.
    """
    self._contours_in_glyph += 1

  def _Handler_pt(self, path, node):
    """Accounts for a new point in the glyph by incrementing the _points_in_glyph field.
    """
    self._points_in_glyph += 1

  def _Handler_assembly(self, path, node):
    """Generates a new TTF program for the node.
    """

    if "fpgm" in path:
      # We want the "fpgm" (Font Program) section empty, as it should only contain instruction/function definitions.
      node.text = ""
    else:
      program = TTProgram(self._twilight_points, self._contours_in_glyph, self._points_in_glyph)
      
      if "prep" in path:
        # Insert a constant number of initialization instructions into the "prep" table.
        node.text = program.GenerateProgram(self.PREP_INSTRUCTIONS)
      else:
        # Generate a regular TrueType program with length depending on the number of glyphs in font.
        node.text = program.GenerateProgram(min(self._num_instructions // self._num_glyphs, self.MAX_INSTRUCTIONS_PER_GLYPH))

  def TraverseNode(self, path, node):
    handlers = {"ttFont" : self._Handler_ttFont,     # global TTF font tag.
                "maxp" : self._Handler_maxp,         # "maxp" TTF table.
                "cvt" : self._Handler_cvt,           # "cvt " TTF table.
                "TTGlyph" : self._Handler_TTGlyph,   # Glyph definition.
                "contour" : self._Handler_contour,   # Contour definition in glyph.
                "pt" : self._Handler_pt,             # Point definition in glyph.
                "assembly" : self._Handler_assembly, # TTF instructions ("fpgm" TTF table, "prep" TTF table or glyph hinting program)
               }

    # Run the tag handler, if one exists.
    if node.tag in handlers:
      handlers[node.tag](path, node)

    # Traverse the XML nodes recursively.
    for child in node:
      self.TraverseNode(path + [node.tag], child)

def main(argv):
  if len(argv) != 4:
    print "Usage: %s /path/to/input.ttx /path/to/output.ttx <number of TT instructions>" % argv[0]
    sys.exit(1)

  tree = ET.parse(argv[1])

  parser = TTXParser(int(argv[3]))
  parser.TraverseNode([], tree.getroot())

  tree.write(argv[2], encoding='utf-8', xml_declaration = True)

if __name__ == "__main__":
  main(sys.argv)
