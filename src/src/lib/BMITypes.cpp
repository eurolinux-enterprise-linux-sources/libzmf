/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BMITypes.h"

namespace libzmf
{

bool operator==(const BMIOffset &lhs, const BMIOffset &rhs)
{
  return lhs.type == rhs.type && lhs.start == rhs.start;
}

bool operator!=(const BMIOffset &lhs, const BMIOffset &rhs)
{
  return !(lhs == rhs);
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
