/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ZMF4PARSER_H_INCLUDED
#define ZMF4PARSER_H_INCLUDED

#include <librevenge/librevenge.h>
#include "libzmf_utils.h"
#include "ZMF4Header.h"
#include "ZMFCollector.h"
#include "ZMFTypes.h"
#include <functional>
#include <vector>
#include <map>
#include <boost/optional.hpp>

namespace libzmf
{

class ZMF4Parser
{
  // disable copying
  ZMF4Parser(const ZMF4Parser &other) = delete;
  ZMF4Parser &operator=(const ZMF4Parser &other) = delete;

public:
  ZMF4Parser(const RVNGInputStreamPtr &input, librevenge::RVNGDrawingInterface *painter);
  bool parse();

private:
  enum class ObjectType
  {
    UNKNOWN,
    FILL,
    TRANSPARENCY,
    PEN,
    SHADOW,
    ARROW,
    FONT,
    PARAGRAPH,
    TEXT,
    BITMAP,
    PAGE_START,
    GUIDELINES,
    PAGE_END,
    LAYER_START,
    LAYER_END,
    DOCUMENT_SETTINGS,
    COLOR_PALETTE,
    RECTANGLE,
    ELLIPSE,
    POLYGON,
    CURVE,
    IMAGE,
    TEXT_FRAME,
    TABLE,
    GROUP_START,
    GROUP_END
  };

  struct ObjectHeader
  {
    ObjectType type;
    uint32_t size;
    uint32_t nextObjectOffset;
    boost::optional<uint32_t> id;
    uint32_t refObjCount;
    uint32_t refListStartOffset;

    ObjectHeader()
      : type()
      , size(0)
      , nextObjectOffset(0)
      , id()
      , refObjCount(0)
      , refListStartOffset(0)
    { }
  };

  struct ObjectRef
  {
    uint32_t id;
    uint32_t tag;
  };

  static ObjectType parseObjectType(uint8_t type);

  ObjectHeader readObjectHeader();

  std::vector<ObjectRef> readObjectRefs();

  boost::optional<Fill> getFillByRefId(uint32_t id);
  boost::optional<Pen> getPenByRefId(uint32_t id);
  boost::optional<Shadow> getShadowByRefId(uint32_t id);
  boost::optional<Transparency> getTransparencyByRefId(uint32_t id);
  boost::optional<Font> getFontByRefId(uint32_t id);
  boost::optional<ParagraphStyle> getParagraphStyleByRefId(uint32_t id);
  boost::optional<Text> getTextByRefId(uint32_t id);
  boost::optional<Image> getImageByRefId(uint32_t id);
  ArrowPtr getArrowByRefId(uint32_t id);

  Style readStyle();

  Point readPoint();
  Point readUnscaledPoint();

  BoundingBox readBoundingBox();

  void readCurveSectionTypes(std::vector<CurveType> &sectionTypes);
  std::vector<Curve> readCurveComponents(std::function<Point()> readPointFunc);

  Color readColor();
  Gradient readGradient(uint32_t type);

  void readPreviewBitmap();

  void readDocumentSettings();
  void readPage();
  void readLayer(const ObjectHeader &layerStartObjHeader);

  void readPen();
  void readFill();
  void readTransparency();
  void readShadow();

  void readArrow();

  void readBitmap();

  void readImage();

  void readFont();
  void readParagraphStyle();
  void readText();

  void readTextFrame();

  void readCurve();
  void readRectangle();
  void readEllipse();
  void readPolygon();

  void readTable();

  const RVNGInputStreamPtr m_input;
  uint32_t m_inputLength;

  ZMFCollector m_collector;

  ZMF4Header m_header;

  ZMFPageSettings m_pageSettings;

  int m_pageNumber;

  ObjectHeader m_currentObjectHeader;

  std::map<uint32_t, Pen> m_pens;
  std::map<uint32_t, Fill> m_fills;
  std::map<uint32_t, Transparency> m_transparencies;
  std::map<uint32_t, Shadow> m_shadows;
  std::map<uint32_t, ArrowPtr> m_arrows;

  std::map<uint32_t, Image> m_images;

  std::map<uint32_t, Font> m_fonts;
  std::map<uint32_t, ParagraphStyle> m_paragraphStyles;
  std::map<uint32_t, Text> m_texts;
};

}

#endif // ZMF4PARSER_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
