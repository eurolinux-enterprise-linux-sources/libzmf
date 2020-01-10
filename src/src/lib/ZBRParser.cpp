/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ZBRParser.h"

namespace libzmf
{

ZBRParser::ZBRParser(const RVNGInputStreamPtr &input, librevenge::RVNGDrawingInterface *painter)
  : m_input(input)
  , m_collector(painter)
  , m_header()
{
}

bool ZBRParser::parse()
{
  if (!m_header.load(m_input) || !m_header.isSupported())
    return false;

  m_collector.startDocument();
  m_collector.endDocument();

  return true;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
