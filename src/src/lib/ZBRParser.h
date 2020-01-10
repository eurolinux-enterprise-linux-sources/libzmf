/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ZBRPARSER_H_INCLUDED
#define ZBRPARSER_H_INCLUDED

#include <librevenge/librevenge.h>

#include "ZBRHeader.h"
#include "ZMFCollector.h"
#include "libzmf_utils.h"

namespace libzmf
{

class ZBRParser
{
  ZBRParser(const ZBRParser &other) = delete;
  ZBRParser &operator=(const ZBRParser &other) = delete;

public:
  ZBRParser(const RVNGInputStreamPtr &input, librevenge::RVNGDrawingInterface *painter);
  bool parse();

protected:
  const RVNGInputStreamPtr m_input;
  ZMFCollector m_collector;
  ZBRHeader m_header;
};

}

#endif // ZBRPARSER_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
