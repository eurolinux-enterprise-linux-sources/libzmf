/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ZMF2HEADER_H_INCLUDED
#define ZMF2HEADER_H_INCLUDED

#include "libzmf_utils.h"

namespace libzmf
{

class ZMF2Header
{
public:
  ZMF2Header();

  bool load(const RVNGInputStreamPtr &input);

  bool isSupported() const;

private:
};

}

#endif // ZMF2HEADER_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
