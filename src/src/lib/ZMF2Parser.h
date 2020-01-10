/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ZMF2PARSER_H_INCLUDED
#define ZMF2PARSER_H_INCLUDED

#include <librevenge/librevenge.h>

#include "libzmf_utils.h"

namespace libzmf
{

class ZMF2Parser
{
  // disable copying
  ZMF2Parser(const ZMF2Parser &other) = delete;
  ZMF2Parser &operator=(const ZMF2Parser &other) = delete;

public:
  ZMF2Parser(const RVNGInputStreamPtr &input, librevenge::RVNGDrawingInterface *painter);
  bool parse();

protected:
  const RVNGInputStreamPtr m_input;
  librevenge::RVNGDrawingInterface *m_painter;
};

}

#endif // ZMF2PARSER_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
