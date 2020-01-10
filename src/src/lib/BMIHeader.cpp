/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BMIHeader.h"
#include <algorithm>

namespace libzmf
{

BMIHeader::BMIHeader()
  : m_signature()
  , m_size(0)
  , m_startOffset(0)
  , m_width(0)
  , m_height(0)
  , m_isPaletteMode(false)
  , m_colorDepth(0)
  , m_offsets()
{
}

bool BMIHeader::load(const RVNGInputStreamPtr &input)
{
  // don't allow multiple calls
  if (m_signature.length() > 0)
    throw GenericException();

  m_startOffset = input->tell();

  unsigned sigLen = 9;
  m_signature.assign(reinterpret_cast<const char *>(readNBytes(input, sigLen)), sigLen);

  m_width = readU16(input);
  m_height = readU16(input);

  m_isPaletteMode = static_cast<bool>(readU16(input));

  m_colorDepth = readU16(input);
  if (!(m_colorDepth == 1 || m_colorDepth == 4 || m_colorDepth == 8 || m_colorDepth == 24))
  {
    ZMF_DEBUG_MSG(("Invalid color depth %d\n", m_colorDepth));
    return false;
  }

  skip(input, 2);

  uint16_t offsetCount = readU16(input);
  if (offsetCount == 0 || offsetCount > 6)
    return false;

  if (m_isPaletteMode)
  {
    skip(input, 4 * paletteColorCount());
  }

  readOffsets(input, offsetCount);

  return true;
}

bool BMIHeader::isSupported() const
{
  return m_signature == "ZonerBMIa";
}
unsigned BMIHeader::size() const
{
  return m_size;
}

unsigned BMIHeader::startOffset() const
{
  return m_startOffset;
}

unsigned BMIHeader::width() const
{
  return m_width;
}

unsigned BMIHeader::height() const
{
  return m_height;
}

bool BMIHeader::isPaletteMode() const
{
  return m_isPaletteMode;
}

unsigned BMIHeader::colorDepth() const
{
  return m_colorDepth;
}

unsigned BMIHeader::paletteColorCount() const
{
  return isPaletteMode() ? (1 << colorDepth()) : 0;
}

const std::vector<BMIOffset> &BMIHeader::offsets() const
{
  return m_offsets;
}

void BMIHeader::readOffsets(const RVNGInputStreamPtr &input, uint16_t offsetCount)
{
  for (unsigned i = 0; i < offsetCount; i++)
  {
    BMIOffset off;

    auto type = readU16(input);
    off.start = readU32(input);

    switch (type)
    {
    case 0x1:
      off.type = BMIStreamType::BITMAP;
      break;
    case 0xff:
      off.type = BMIStreamType::END_OF_FILE;
      m_size = off.start;
      break;
    default:
      off.type = BMIStreamType::UNKNOWN;
      break;
    }

    m_offsets.push_back(off);
  }

  std::sort(m_offsets.begin(), m_offsets.end(), [](const BMIOffset &a, const BMIOffset &b)
  {
    return (a.start < b.start);
  });

  m_offsets.erase(std::unique(m_offsets.begin(), m_offsets.end()), m_offsets.end());

  for (unsigned i = 0; i < m_offsets.size() - 1; i++)
  {
    m_offsets[i].end = m_offsets[i + 1].start;
  }
}

namespace
{

bool reconcileValue(unsigned &v1, unsigned &v2, unsigned &v3)
{
  if (v1 == v2)
  {
    if (v2 != v3)
      v3 = v1;
  }
  else if (v1 == v3)
  {
    v2 = v1;
  }
  else if (v2 == v3)
  {
    v1 = v2;
  }
  else
  {
    return false;
  }
  return true;
}

}

bool BMIHeader::reconcileWidth(unsigned &colorWidth, unsigned &transparencyWidth)
{
  return reconcileValue(m_width, colorWidth, transparencyWidth);
}

bool BMIHeader::reconcileHeight(unsigned &colorHeight, unsigned &transparencyHeight)
{
  return reconcileValue(m_height, colorHeight, transparencyHeight);
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
