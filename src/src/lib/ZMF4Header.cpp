/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ZMF4Header.h"


#define ZMF4_SIGNATURE 0x12345678


namespace libzmf
{

ZMF4Header::ZMF4Header()
  : m_signature(0)
  , m_objectCount(0)
  , m_startContentOffset(0)
  , m_startBitmapOffset(0)
{
}

bool ZMF4Header::load(const RVNGInputStreamPtr &input)
{
  seek(input, 8);

  m_signature = readU32(input);

  if (!checkSignature())
    return false;

  seek(input, 28);

  m_objectCount = readU32(input);

  m_startContentOffset = readU32(input);

  m_startBitmapOffset = readU32(input);

  return true;
}

bool ZMF4Header::isSupported() const
{
  return checkSignature();
}

uint32_t ZMF4Header::objectCount() const
{
  return m_objectCount;
}

uint32_t ZMF4Header::startContentOffset() const
{
  return m_startContentOffset;
}

uint32_t ZMF4Header::startBitmapOffset() const
{
  return m_startBitmapOffset;
}

bool ZMF4Header::checkSignature() const
{
  return m_signature == ZMF4_SIGNATURE;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
