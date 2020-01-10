/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ZBRHeader.h"

#define ZBR_SIG 0x29a

namespace libzmf
{

ZBRHeader::ZBRHeader()
  : m_sig(0)
  , m_version(0)
{
}

bool ZBRHeader::load(const RVNGInputStreamPtr &input) try
{
  m_sig = readU16(input);
  m_version = readU16(input);
  skip(input, 100);
  return true;
}
catch (...)
{
  return false;
}

bool ZBRHeader::isSupported() const
{
  return m_sig == ZBR_SIG && m_version < 5;
}

unsigned ZBRHeader::version() const
{
  return m_version;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
