/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef BMIPARSER_H_INCLUDED
#define BMIPARSER_H_INCLUDED

#include <librevenge/librevenge.h>

#include "libzmf_utils.h"

#include "BMIHeader.h"
#include "ZMFTypes.h"

namespace libzmf
{

class BMIParser
{
  // disable copying
  BMIParser(const BMIParser &other) = delete;
  BMIParser &operator=(const BMIParser &other) = delete;

  struct ColorBitmapHeader;

public:
  explicit BMIParser(const RVNGInputStreamPtr &input, librevenge::RVNGDrawingInterface *painter = 0);

  /** Parse the bitmap and output it as a drawing doc with an inserted image.
    */
  bool parse();

  /** Parse the bitmap and convert it to PNG.
    */
  Image readImage();

  const BMIHeader &header() const;

private:
  ColorBitmap readColorBitmap(const ColorBitmapHeader &header);

  std::vector<uint8_t> readData(unsigned endOffset);

  std::vector<Color> readColorPalette(unsigned colorDepth);

  bool reconcileDimensions(ColorBitmapHeader &colorHeader, ColorBitmapHeader &transparencyHeader);

  const RVNGInputStreamPtr m_input;
  librevenge::RVNGDrawingInterface *m_painter;

  BMIHeader m_header;
};

}

#endif // BMIPARSER_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
