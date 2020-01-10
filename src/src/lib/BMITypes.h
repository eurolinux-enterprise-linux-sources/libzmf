/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef BMITYPES_H_INCLUDED
#define BMITYPES_H_INCLUDED

#include "libzmf_utils.h"
#include "ZMFTypes.h"
#include <vector>

namespace libzmf
{

enum class BMIStreamType
{
  UNKNOWN,
  BITMAP,
  END_OF_FILE
};

struct BMIOffset
{
  BMIStreamType type;
  uint32_t start;
  uint32_t end;

  BMIOffset()
    : type(BMIStreamType::UNKNOWN), start(0), end(0)
  { }
};

bool operator==(const BMIOffset &lhs, const BMIOffset &rhs);
bool operator!=(const BMIOffset &lhs, const BMIOffset &rhs);

struct ColorBitmap
{
  uint32_t width;
  uint32_t height;
  std::vector<Color> data;

  ColorBitmap()
    : width(0), height(0), data()
  { }
};

}

#endif // BMITYPES_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
