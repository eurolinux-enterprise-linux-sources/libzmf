/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ZMF2Header.h"

namespace libzmf
{

ZMF2Header::ZMF2Header()
{
}

bool ZMF2Header::load(const RVNGInputStreamPtr &input)
{
  // TODO: implement me
  (void) input;
  return false;
}

bool ZMF2Header::isSupported() const
{
  // TODO: implement me
  return false;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
