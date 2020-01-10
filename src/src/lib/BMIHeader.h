/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef BMIHEADER_H_INCLUDED
#define BMIHEADER_H_INCLUDED

#include "libzmf_utils.h"
#include "BMITypes.h"
#include <vector>

namespace libzmf
{

class BMIHeader
{
public:
  BMIHeader();

  bool load(const RVNGInputStreamPtr &input);

  bool isSupported() const;

  unsigned size() const;

  unsigned startOffset() const;

  unsigned width() const;
  unsigned height() const;

  bool isPaletteMode() const;

  unsigned colorDepth() const;

  unsigned paletteColorCount() const;

  const std::vector<BMIOffset> &offsets() const;

  bool reconcileWidth(unsigned &colorWidth, unsigned &transparencyWidth);
  bool reconcileHeight(unsigned &colorHeight, unsigned &transparencyHeight);

private:
  void readOffsets(const RVNGInputStreamPtr &input, uint16_t offsetCount);

  std::string m_signature;

  unsigned m_size;

  unsigned m_startOffset;

  unsigned m_width;
  unsigned m_height;

  bool m_isPaletteMode;

  unsigned m_colorDepth;

  std::vector<BMIOffset> m_offsets;
};

}

#endif // BMIHEADER_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
