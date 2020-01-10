/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cassert>

#include <libzmf/libzmf.h>

#include "BMIHeader.h"
#include "BMIParser.h"
#include "ZBRHeader.h"
#include "ZBRParser.h"
#include "ZMF4Header.h"
#include "ZMF4Parser.h"
#include "libzmf_utils.h"

namespace libzmf
{

namespace
{

struct DetectionInfo
{
  DetectionInfo();

  RVNGInputStreamPtr m_content;
  RVNGInputStreamPtr m_package;
  ZMFDocument::Type m_type;
  ZMFDocument::Kind m_kind;
};

DetectionInfo::DetectionInfo()
  : m_content()
  , m_package()
  , m_type(ZMFDocument::TYPE_UNKNOWN)
  , m_kind(ZMFDocument::KIND_UNKNOWN)
{
}

template<typename THeader>
bool detectFormat(DetectionInfo &info, const ZMFDocument::Type type, const ZMFDocument::Kind kind) try
{
  seek(info.m_content, 0);
  THeader header;
  if (header.load(info.m_content) && header.isSupported())
  {
    info.m_type = type;
    info.m_kind = kind;
    return true;
  }
  return false;
}
catch (...)
{
  return false;
}

bool detect(const RVNGInputStreamPtr &input, DetectionInfo &info)
{
  if (input->isStructured())
  {
    info.m_package = input;
    if (input->existsSubStream("content.zmf"))
    {
      info.m_content.reset(input->getSubStreamByName("content.zmf"));
      return detectFormat<ZMF4Header>(info, ZMFDocument::TYPE_DRAW, ZMFDocument::KIND_DRAW);
    }
  }
  else
  {
    info.m_content = input;
    return detectFormat<ZMF4Header>(info, ZMFDocument::TYPE_DRAW, ZMFDocument::KIND_DRAW)
           || detectFormat<BMIHeader>(info, ZMFDocument::TYPE_BITMAP, ZMFDocument::KIND_PAINT)
           || detectFormat<ZBRHeader>(info, ZMFDocument::TYPE_ZEBRA, ZMFDocument::KIND_DRAW)
           ;
  }
  return false;
}

}

bool ZMFDocument::isSupported(librevenge::RVNGInputStream *const input, Type *const type, Kind *const kind) try
{
  DetectionInfo info;
  if (detect(RVNGInputStreamPtr(input, ZMFDummyDeleter()), info))
  {
    assert(bool(info.m_content));
    if (type)
      *type = info.m_type;
    if (kind)
      *kind = info.m_kind;
    return true;
  }
  return false;
}
catch (...)
{
  return false;
}

bool ZMFDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter) try
{
  DetectionInfo info;
  if (!detect(RVNGInputStreamPtr(input, ZMFDummyDeleter()), info))
    return false;

  info.m_content->seek(0, librevenge::RVNG_SEEK_SET);

  switch (info.m_type)
  {
  case TYPE_DRAW:
  {
    ZMF4Parser parser(info.m_content, painter);
    return parser.parse();
  }
  case TYPE_ZEBRA:
  {
    ZBRParser parser(info.m_content, painter);
    return parser.parse();
  }
  case TYPE_BITMAP:
  {
    BMIParser parser(info.m_content, painter);
    return parser.parse();
  }
  default:
    break;
  }

  return false;
}
catch (...)
{
  return false;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
