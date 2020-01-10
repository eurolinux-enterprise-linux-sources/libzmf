/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ZMF4Parser.h"
#include <algorithm>
#include <numeric>
#include <string>
#include "BMIParser.h"

#define NO_ID 0xffffffff

namespace libzmf
{

namespace
{

template<typename T>
boost::optional<T> getByRefId(uint32_t id, const std::map<uint32_t, T> &idObjMap)
{
  if (id == NO_ID)
  {
    return boost::optional<T>();
  }
  if (idObjMap.find(id) != idObjMap.end())
  {
    return idObjMap.at(id);
  }
  else
  {
    ZMF_DEBUG_MSG(("%s with ID 0x%x not found\n", prettyTypeName<T>().c_str(), id));
    return boost::optional<T>();
  }
}

}

ZMF4Parser::ZMF4Parser(const RVNGInputStreamPtr &input, librevenge::RVNGDrawingInterface *painter)
  : m_input(input)
  , m_inputLength(0)
  , m_collector(painter)
  , m_header()
  , m_pageSettings()
  , m_pageNumber(0)
  , m_currentObjectHeader()
  , m_pens()
  , m_fills()
  , m_transparencies()
  , m_shadows()
  , m_arrows()
  , m_images()
  , m_fonts()
  , m_paragraphStyles()
  , m_texts()
{
  // fill with ID 0x3 used by default for text (black)
  m_fills[0x3] = Color(0, 0, 0);

  // pen with ID 0x1 is used for borders in table cells, rows and columns when they have no border
  // (0xffffffff aka None probably not used because it would not override column/row pen)
  Pen pen(Color(255, 255, 255));
  pen.isInvisible = true;
  m_pens[0x1] = pen;
}

bool ZMF4Parser::parse()
{
  m_inputLength = getLength(m_input);

  if (!m_header.load(m_input) || !m_header.isSupported())
    return false;

  m_collector.startDocument();

  if (m_header.startBitmapOffset() > 0)
  {
    seek(m_input, m_header.startBitmapOffset());

    readPreviewBitmap();
  }
  else
  {
    seek(m_input, m_header.startContentOffset());
  }

  readDocumentSettings();

  while (!m_input->isEnd())
  {
    readPage();
  }

  m_collector.endDocument();

  return true;
}

ZMF4Parser::ObjectType ZMF4Parser::parseObjectType(uint8_t type)
{
  switch (type)
  {
  case 0xa:
    return ObjectType::FILL;
  case 0xb:
    return ObjectType::TRANSPARENCY;
  case 0xc:
    return ObjectType::PEN;
  case 0xd:
    return ObjectType::SHADOW;
  case 0xf:
    return ObjectType::ARROW;
  case 0x10:
    return ObjectType::FONT;
  case 0x11:
    return ObjectType::PARAGRAPH;
  case 0x12:
    return ObjectType::TEXT;
  case 0xe:
    return ObjectType::BITMAP;
  case 0x21:
    return ObjectType::PAGE_START;
  case 0x22:
    return ObjectType::GUIDELINES;
  case 0x23:
    return ObjectType::PAGE_END;
  case 0x24:
    return ObjectType::LAYER_START;
  case 0x25:
    return ObjectType::LAYER_END;
  case 0x27:
    return ObjectType::DOCUMENT_SETTINGS;
  case 0x28:
    return ObjectType::COLOR_PALETTE;
  case 0x32:
    return ObjectType::RECTANGLE;
  case 0x33:
    return ObjectType::ELLIPSE;
  case 0x34:
    return ObjectType::POLYGON;
  case 0x36:
    return ObjectType::CURVE;
  case 0x37:
    return ObjectType::IMAGE;
  case 0x3a:
    return ObjectType::TEXT_FRAME;
  case 0x3b:
    return ObjectType::TABLE;
  case 0x41:
    return ObjectType::GROUP_START;
  case 0x42:
    return ObjectType::GROUP_END;
  default:
    ZMF_DEBUG_MSG(("Unknown object type 0x%x\n", type));
    return ObjectType::UNKNOWN;
  }
}

ZMF4Parser::ObjectHeader ZMF4Parser::readObjectHeader()
{
  uint32_t startOffset = m_input->tell();

  ObjectHeader header;

  header.size = readU32(m_input);
  header.type = parseObjectType(readU8(m_input));

  skip(m_input, 7);

  header.refObjCount = readU32(m_input);
  header.refListStartOffset = readU32(m_input);

  if (header.size == 0 || header.size > m_inputLength - startOffset ||
      header.refListStartOffset >= header.size ||
      header.refObjCount > (header.size - header.refListStartOffset) / 8)
  {
    ZMF_DEBUG_MSG(("Incorrect object header, offset %u\n", startOffset));
    throw GenericException();
  }

  skip(m_input, 4);

  uint32_t id = readU32(m_input);
  if (id != NO_ID)
  {
    header.id = id;
  }

  header.nextObjectOffset = startOffset + header.size;
  if (header.refListStartOffset > 0)
    header.refListStartOffset += startOffset;

  return header;
}

std::vector<ZMF4Parser::ObjectRef> ZMF4Parser::readObjectRefs()
{
  uint32_t refObjCount = m_currentObjectHeader.refObjCount;
  long maxRefObjCount = long(static_cast<unsigned long>(m_currentObjectHeader.nextObjectOffset)) - m_input->tell() / 8;
  if (maxRefObjCount < 0)
    maxRefObjCount = 0;
  if (refObjCount > maxRefObjCount)
    refObjCount = uint32_t(static_cast<unsigned long>(maxRefObjCount));

  std::vector<ZMF4Parser::ObjectRef> refs;

  if (refObjCount > 0)
  {
    refs.resize(refObjCount);
    seek(m_input, m_currentObjectHeader.nextObjectOffset - 8 * refObjCount);

    for (uint32_t i = 0; i < refObjCount; i++)
    {
      refs[i].id = readU32(m_input);
    }

    for (uint32_t i = 0; i < refObjCount; i++)
    {
      refs[i].tag = readU32(m_input);
    }

    refs.erase(std::remove_if(refs.begin(), refs.end(),
                              [](const ObjectRef &ref)
    {
      return ref.id == NO_ID;
    }), refs.end());
  }

  return refs;
}

boost::optional<Fill> ZMF4Parser::getFillByRefId(uint32_t id)
{
  return getByRefId<Fill>(id, m_fills);
}

boost::optional<Pen> ZMF4Parser::getPenByRefId(uint32_t id)
{
  return getByRefId<Pen>(id, m_pens);
}

boost::optional<Shadow> ZMF4Parser::getShadowByRefId(uint32_t id)
{
  return getByRefId<Shadow>(id, m_shadows);
}

boost::optional<Transparency> ZMF4Parser::getTransparencyByRefId(uint32_t id)
{
  return getByRefId<Transparency>(id, m_transparencies);
}

boost::optional<Font> ZMF4Parser::getFontByRefId(uint32_t id)
{
  return getByRefId<Font>(id, m_fonts);
}

boost::optional<ParagraphStyle> ZMF4Parser::getParagraphStyleByRefId(uint32_t id)
{
  return getByRefId<ParagraphStyle>(id, m_paragraphStyles);
}

boost::optional<Text> ZMF4Parser::getTextByRefId(uint32_t id)
{
  return getByRefId<Text>(id, m_texts);
}

boost::optional<Image> ZMF4Parser::getImageByRefId(uint32_t id)
{
  return getByRefId<Image>(id, m_images);
}

ArrowPtr ZMF4Parser::getArrowByRefId(uint32_t id)
{
  auto arrow = getByRefId<ArrowPtr>(id, m_arrows);
  return get_optional_value_or(arrow, ArrowPtr());
}

Style ZMF4Parser::readStyle()
{
  auto refs = readObjectRefs();

  Style style;

  for (const auto &ref : refs)
  {
    switch (ref.tag)
    {
    case 1:
      style.fill = getFillByRefId(ref.id);
      break;
    case 2:
      style.pen = getPenByRefId(ref.id);
      break;
    case 3:
      style.shadow = getShadowByRefId(ref.id);
      break;
    case 4:
      style.transparency = getTransparencyByRefId(ref.id);
      break;
    default:
      break;
    }
  }

  return style;
}

Point ZMF4Parser::readPoint()
{
  double x = um2in(readS32(m_input));
  double y = um2in(readS32(m_input));
  return Point(x, y);
}

Point ZMF4Parser::readUnscaledPoint()
{
  double x = readFloat(m_input);
  double y = readFloat(m_input);
  return Point(x, y);
}

BoundingBox ZMF4Parser::readBoundingBox()
{
  // width and height may not be correct in some cases
  // for example looks like it's not updated when resizing objects
  skip(m_input, 8);

  std::vector<Point> points;

  for (uint32_t i = 0; i < 4; i++)
  {
    points.push_back(readPoint());
  }

#ifdef DEBUG_DRAW_BBOX
  Curve curve;
  curve.points = points;
  curve.sectionTypes = std::vector<CurveType>(curve.points.size() - 1, CurveType::LINE);
  curve.closed = true;

  Style style;
  style.pen = Pen(Color(0, 0, 255));
  m_collector.setStyle(style);

  m_collector.collectPath(curve);
#endif

  return BoundingBox(points);
}

void ZMF4Parser::readCurveSectionTypes(std::vector<CurveType> &sectionTypes)
{
  while (true)
  {
    uint32_t type = readU32(m_input);
    switch (type)
    {
    default:
      ZMF_DEBUG_MSG(("Unknown point type %u\n", type));
    // fall-through intended: pick a default
    case 1:
      sectionTypes.push_back(CurveType::LINE);
      break;
    case 2:
      sectionTypes.push_back(CurveType::BEZIER_CURVE);
      skip(m_input, 8);
      break;
    case 0x64:
      return;
    }
  }
}

std::vector<Curve> ZMF4Parser::readCurveComponents(std::function<Point()> readPointFunc)
{
  uint32_t componentsCount = readU32(m_input);
  if (componentsCount == 0 || componentsCount > 10000) // just a random big number
  {
    ZMF_DEBUG_MSG(("Incorrect curve component count, offset %ld\n", m_input->tell()));
    return std::vector<Curve>();
  }

  std::vector<Curve> curves(componentsCount);

  for (uint32_t i = 0; i < componentsCount; i++)
  {
    skip(m_input, 8);

    uint32_t componentPointsCount = readU32(m_input);
    if (componentPointsCount == 0 || componentPointsCount > 10000) // just a random big number
    {
      ZMF_DEBUG_MSG(("Incorrect curve point count, offset %ld\n", m_input->tell()));
      return std::vector<Curve>();
    }

    curves[i].points.resize(componentPointsCount);
    curves[i].sectionTypes.reserve(componentPointsCount - 1);

    curves[i].closed = static_cast<bool>(readU32(m_input));
  }

  for (auto &curve : curves)
  {
    for (size_t i = 0; i < curve.points.size(); i++)
    {
      curve.points[i] = readPointFunc();
    }
  }

  for (auto &curve : curves)
  {
    readCurveSectionTypes(curve.sectionTypes);
  }

  return curves;
}

Color ZMF4Parser::readColor()
{
  Color color;
  color.red = readU8(m_input);
  color.green = readU8(m_input);
  color.blue = readU8(m_input);
  return color;
}

Gradient ZMF4Parser::readGradient(uint32_t type)
{
  Gradient gradient;

  switch (type)
  {
  case 2:
    gradient.type = GradientType::LINEAR;
    break;
  case 3:
    gradient.type = GradientType::RADIAL;
    break;
  case 4:
    gradient.type = GradientType::CONICAL;
    break;
  case 5:
    gradient.type = GradientType::CROSS;
    break;
  case 6:
    gradient.type = GradientType::RECTANGULAR;
    break;
  case 7:
    gradient.type = GradientType::FLEXIBLE;
    break;
  default:
    ZMF_DEBUG_MSG(("unknown gradient type %u\n", type));
    gradient.type = GradientType::LINEAR;
    break;
  }

  skip(m_input, 4);

  uint32_t stopCount = readU32(m_input);
  if (m_input->tell() + 20 + stopCount * 16 > m_currentObjectHeader.nextObjectOffset ||
      m_input->tell() + 20 + stopCount * 16 < stopCount) // overflow
  {
    ZMF_DEBUG_MSG(("Incorrect gradient stop count, offset %ld\n", m_input->tell()));
    return gradient;
  }

  skip(m_input, 4);

  gradient.center.x = readFloat(m_input);
  gradient.center.y = readFloat(m_input);

  gradient.angle = readFloat(m_input);

  skip(m_input, 4);

  gradient.stops.reserve(stopCount);
  for (uint32_t i = 0; i < stopCount; i++)
  {
    GradientStop stop;

    skip(m_input, 4);
    stop.color = readColor();

    skip(m_input, 5);
    stop.offset = readFloat(m_input);

    gradient.stops.push_back(stop);
  }

  return gradient;
}

void ZMF4Parser::readPreviewBitmap()
{
  skip(m_input, 2);

  uint32_t size = readU32(m_input);

  skip(m_input, size - 2 - 4);
}

void ZMF4Parser::readDocumentSettings()
{
  ObjectHeader header = readObjectHeader();

  if (header.type != ObjectType::DOCUMENT_SETTINGS)
    throw GenericException();

  skip(m_input, 32);

  Color color = readColor();

  skip(m_input, 5);

  double pageWidth = um2in(readU32(m_input));
  double pageHeight = um2in(readU32(m_input));

  skip(m_input, 68);

  double leftOffset = um2in(readU32(m_input));
  double topOffset = um2in(readU32(m_input));

  m_pageSettings = ZMFPageSettings(pageWidth, pageHeight, leftOffset, topOffset, color);

  seek(m_input, header.nextObjectOffset);
}

void ZMF4Parser::readPage()
{
  ObjectHeader startPageHeader;

  // skip color palettes, all used colors are included in Fill/Pen/...
  while (true)
  {
    startPageHeader = readObjectHeader();
    if (startPageHeader.type == ObjectType::COLOR_PALETTE)
    {
      seek(m_input, startPageHeader.nextObjectOffset);
    }
    else
    {
      break;
    }
  }

  if (startPageHeader.type != ObjectType::PAGE_START)
    throw GenericException();

  m_pageNumber++;

  // skip first empty/master page
  if (m_pageNumber == 1)
  {
    do
    {
      seek(m_input, startPageHeader.nextObjectOffset);
      startPageHeader = readObjectHeader();
    }
    while (startPageHeader.type != ObjectType::PAGE_START);
  }

  m_collector.startPage(m_pageSettings);

  seek(m_input, startPageHeader.nextObjectOffset);

  while (true)
  {
    ObjectHeader header = readObjectHeader();

    switch (header.type)
    {
    case ObjectType::GUIDELINES:
      seek(m_input, header.nextObjectOffset);
      break;
    case ObjectType::PAGE_END:
      m_collector.endPage();
      if (!m_input->isEnd())
        seek(m_input, header.nextObjectOffset);
      return;
    case ObjectType::LAYER_START:
      readLayer(header);
      break;
    default:
      throw GenericException();
    }
  }
}

void ZMF4Parser::readLayer(const ZMF4Parser::ObjectHeader &layerStartObjHeader)
{
  if (layerStartObjHeader.type != ObjectType::LAYER_START)
    throw GenericException();

  m_collector.startLayer();

  seek(m_input, layerStartObjHeader.nextObjectOffset);

  while (true)
  {
    m_currentObjectHeader = readObjectHeader();

    switch (m_currentObjectHeader.type)
    {
    case ObjectType::LAYER_END:
      seek(m_input, m_currentObjectHeader.nextObjectOffset);
      m_collector.endLayer();
      return;
    case ObjectType::FILL:
      readFill();
      break;
    case ObjectType::TRANSPARENCY:
      readTransparency();
      break;
    case ObjectType::PEN:
      readPen();
      break;
    case ObjectType::SHADOW:
      readShadow();
      break;
    case ObjectType::ARROW:
      readArrow();
      break;
    case ObjectType::FONT:
      readFont();
      break;
    case ObjectType::PARAGRAPH:
      readParagraphStyle();
      break;
    case ObjectType::TEXT:
      readText();
      break;
    case ObjectType::BITMAP:
      readBitmap();
      break;
    case ObjectType::RECTANGLE:
      readRectangle();
      break;
    case ObjectType::ELLIPSE:
      readEllipse();
      break;
    case ObjectType::POLYGON:
      readPolygon();
      break;
    case ObjectType::CURVE:
      readCurve();
      break;
    case ObjectType::IMAGE:
      readImage();
      break;
    case ObjectType::TEXT_FRAME:
      readTextFrame();
      break;
    case ObjectType::TABLE:
      readTable();
      break;
    case ObjectType::GROUP_START:
      m_collector.startGroup();
      break;
    case ObjectType::GROUP_END:
      m_collector.endGroup();
      break;
    default:
      break;
    }

    if (m_currentObjectHeader.type != ObjectType::BITMAP)
    {
      seek(m_input, m_currentObjectHeader.nextObjectOffset);
    }
  }
}

void ZMF4Parser::readFill()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Fill without ID, offset %ld\n", m_input->tell()));
    return;
  }

  skip(m_input, 8);

  uint32_t type = readU32(m_input);

  if (type == 1)
  {
    skip(m_input, 8);

    Color color = readColor();

    m_fills[get(m_currentObjectHeader.id)] = color;
  }
  else if (type >= 2 && type <= 7)
  {
    Gradient gradient = readGradient(type);

    m_fills[get(m_currentObjectHeader.id)] = gradient;
  }
  else if (type == 8)
  {
    ImageFill imageFill;

    skip(m_input, 4);

    imageFill.tile = static_cast<bool>(readU32(m_input));
    imageFill.tileWidth = um2in(readU32(m_input));
    imageFill.tileHeight = um2in(readU32(m_input));

    boost::optional<Image> image;

    auto refs = readObjectRefs();

    for (const auto &ref : refs)
    {
      switch (ref.tag)
      {
      case 0:
      {
        image = getImageByRefId(ref.id);
      }
      break;
      default:
        ZMF_DEBUG_MSG(("unknown bitmap fill ref tag %u\n", ref.tag));
        break;
      }
    }

    if (!image)
    {
      ZMF_DEBUG_MSG(("image not found for bitmap fill ID0x%x\n", get(m_currentObjectHeader.id)));
      return;
    }

    imageFill.image = get(image);

    m_fills[get(m_currentObjectHeader.id)] = imageFill;
  }
  else
  {
    ZMF_DEBUG_MSG(("unknown fill type %u\n", type));
  }
}

void ZMF4Parser::readTransparency()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Transparency without ID, offset %ld\n", m_input->tell()));
    return;
  }

  skip(m_input, 8);

  uint32_t type = readU32(m_input);

  if (type == 1)
  {
    skip(m_input, 8);

    Transparency transparency;
    transparency.color = readColor();

    m_transparencies[get(m_currentObjectHeader.id)] = transparency;
  }
}

void ZMF4Parser::readPen()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Pen without ID, offset %ld\n", m_input->tell()));
    return;
  }

  Pen pen;

  skip(m_input, 12);

  uint32_t lineJoin = readU32(m_input);

  switch (lineJoin)
  {
  default:
    ZMF_DEBUG_MSG(("Unknown line join type %u\n", lineJoin));
  // fall-through intended: pick a default
  case 0:
    pen.lineJoinType = LineJoinType::MITER;
    break;
  case 1:
    pen.lineJoinType = LineJoinType::ROUND;
    break;
  case 2:
    pen.lineJoinType = LineJoinType::BEVEL;
    break;
  }

  uint32_t lineCap = readU32(m_input);

  switch (lineCap)
  {
  default:
    ZMF_DEBUG_MSG(("Unknown line cap type %u\n", lineCap));
  // fall-through intended: pick a default
  case 0:
    pen.lineCapType = LineCapType::BUTT;
    break;
  case 1:
    pen.lineCapType = LineCapType::FLAT;
    break;
  case 2:
    pen.lineCapType = LineCapType::ROUND;
    break;
  case 3:
    pen.lineCapType = LineCapType::POINTED;
    break;
  }

  skip(m_input, 4);

  pen.width = um2in(readU32(m_input));

  skip(m_input, 4);

  pen.color = readColor();

  skip(m_input, 17);

  auto dashBits = bytesToBitset<6>(readNBytes(m_input, 6));
  uint16_t dashLength = readU16(m_input);

  if (!dashBits.all())
  {
    int curLength = 1;
    bool prevVal = true;
    // sometimes the first bit of the 4th byte is set
    // not sure what it means but librevenge/ODF don't support complex patterns anyway,
    // so 3 bytes should be enough
    for (std::size_t i = 1; i < 24; i++)
    {
      if (dashBits[i] != prevVal)
      {
        pen.dashPattern.push_back(curLength);
        curLength = 0;
      }

      curLength++;
      prevVal = dashBits[i];
    }

    int sum = static_cast<int>(std::accumulate(pen.dashPattern.begin(), pen.dashPattern.end(), 0));
    // not sure how to properly use length
    // looks like it is length of all used "dots" (bits), empty or not,
    // and each dot usually is 1024 of whatever units it is
    // so we can easily get number of empty dots at the end
    // (sometimes it is a bit bigger or smaller than 1024 though)
    pen.dashDistance = (dashLength / 1024) - sum;
    if (pen.dashDistance < 1)
    {
      pen.dashDistance = 1;
    }
  }

  auto refs = readObjectRefs();

  for (const auto &ref : refs)
  {
    switch (ref.tag)
    {
    case 0:
      pen.startArrow = getArrowByRefId(ref.id);
      break;
    case 1:
      pen.endArrow = getArrowByRefId(ref.id);
      break;
    default:
      break;
    }
  }

  m_pens[get(m_currentObjectHeader.id)] = pen;
}

void ZMF4Parser::readShadow()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Shadow without ID, offset %ld\n", m_input->tell()));
    return;
  }

  Shadow shadow;

  skip(m_input, 8);

  uint32_t type = readU32(m_input);

  shadow.offset = readPoint();
  shadow.angle = readFloat(m_input);

  switch (type)
  {
  case 1: // Color
  case 3: // Soft
  {
    skip(m_input, 4);

    shadow.color = readColor();

    if (type == 1)
      break;

    skip(m_input, 5);

    shadow.opacity = 1.0 - readFloat(m_input);

    // todo: blur
  }
  break;
  case 2: // Brightness
  case 4: // Transparent
  {
    shadow.opacity = 1.0 - readFloat(m_input);

    // todo: ...
  }
  break;
  default:
    ZMF_DEBUG_MSG(("Unknown shadow type %u\n", type));
    break;
  }

  m_shadows[get(m_currentObjectHeader.id)] = shadow;
}

void ZMF4Parser::readArrow()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Arrow without ID, offset %ld\n", m_input->tell()));
    return;
  }

  ArrowPtr arrow(new Arrow);

  skip(m_input, 4);

  arrow->lineEndX = readFloat(m_input);

  skip(m_input, 12);

  arrow->curves = readCurveComponents(std::bind(&ZMF4Parser::readUnscaledPoint, this));

  m_arrows[get(m_currentObjectHeader.id)] = arrow;
}

void ZMF4Parser::readBitmap()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Bitmap without ID, offset %ld\n", m_input->tell()));
    return;
  }

  skip(m_input, 4);

  uint32_t something = readU32(m_input);
  bool hasData = static_cast<bool>(something);

  seek(m_input, m_currentObjectHeader.nextObjectOffset);

  if (hasData)
  {
    BMIParser parser(m_input);

    auto image = parser.readImage();

    const auto &header = parser.header();

    if (image.data.empty())
    {
      ZMF_DEBUG_MSG(("Failed to parse bitmap, ID 0x%x\n", get(m_currentObjectHeader.id)));
    }
    else
    {
      m_images[get(m_currentObjectHeader.id)] = image;
    }

    seek(m_input, m_currentObjectHeader.nextObjectOffset + header.size());
  }
}

void ZMF4Parser::readImage()
{
  auto bbox = readBoundingBox();

  boost::optional<Image> image;

  auto refs = readObjectRefs();

  for (const auto &ref : refs)
  {
    switch (ref.tag)
    {
    case 5:
    {
      image = getImageByRefId(ref.id);
    }
    break;
    default:
      break;
    }
  }

  if (image)
  {
    m_collector.setStyle(readStyle());

    m_collector.collectImage(get(image).data, bbox.topLeft(), bbox.width(), bbox.height(),
                             bbox.rotation(), bbox.mirrorHorizontal(), bbox.mirrorVertical());
  }
}

void ZMF4Parser::readFont()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Font without ID, offset %ld\n", m_input->tell()));
    return;
  }

  skip(m_input, 4);

  Font font;

  auto formatFlags = readU8(m_input);

  font.isBold = formatFlags & 0x1;
  font.isItalic = formatFlags & 0x2;

  skip(m_input, 3);

  font.size = readFloat(m_input);

  skip(m_input, 4);

  const std::string name(reinterpret_cast<const char *>(readNBytes(m_input, 32)), 31);
  font.name = librevenge::RVNGString(name.c_str());

  auto style = readStyle();
  font.fill = style.fill;
  font.outline = style.pen;

  m_fonts[get(m_currentObjectHeader.id)] = font;
}

void ZMF4Parser::readParagraphStyle()
{
  if (!m_currentObjectHeader.id)
  {
    ZMF_DEBUG_MSG(("Paragraph without ID, offset %ld\n", m_input->tell()));
    return;
  }

  skip(m_input, 4);

  ParagraphStyle parStyle;

  auto align = readU8(m_input);

  switch (align)
  {
  default:
    ZMF_DEBUG_MSG(("Unknown paragraph alignment %u\n", align));
  // fall-through intended: pick a default
  case 0:
    parStyle.alignment = HorizontalAlignment::LEFT;
    break;
  case 1:
    parStyle.alignment = HorizontalAlignment::RIGHT;
    break;
  case 2:
    parStyle.alignment = HorizontalAlignment::BLOCK;
    break;
  case 3:
    parStyle.alignment = HorizontalAlignment::CENTER;
    break;
  case 4:
    parStyle.alignment = HorizontalAlignment::FULL;
    break;
  }

  skip(m_input, 3);

  parStyle.lineSpacing = readFloat(m_input);

  auto refs = readObjectRefs();

  for (const auto &ref : refs)
  {
    switch (ref.tag)
    {
    case 1:
    {
      auto font = getFontByRefId(ref.id);
      if (font)
        parStyle.font = get(font);
    }
    break;
    default:
      ZMF_DEBUG_MSG(("unknown paragraph ref tag %u\n", ref.tag));
      break;
    }
  }

  m_paragraphStyles[get(m_currentObjectHeader.id)] = parStyle;
}

void ZMF4Parser::readText()
{
  skip(m_input, 12);

  uint32_t paragraphCount = readU32(m_input);
  if (paragraphCount == 0 || paragraphCount > 1000) // just a random big number
  {
    ZMF_DEBUG_MSG(("Incorrect paragraph count, offset %ld\n", m_input->tell()));
    return;
  }

  Text text;

  text.paragraphs.resize(paragraphCount);

  skip(m_input, 4);

  for (auto &paragraph : text.paragraphs)
  {
    uint32_t spanCount = readU32(m_input);
    if (spanCount > 1000) // just a random big number
    {
      ZMF_DEBUG_MSG(("Incorrect span count, offset %ld\n", m_input->tell()));
      return;
    }
    paragraph.spans.resize(spanCount);

    uint32_t styleId = readU32(m_input);
    auto style = getParagraphStyleByRefId(styleId);
    if (style)
      paragraph.style = get(style);

    skip(m_input, 4);
  }

  for (auto &paragraph : text.paragraphs)
  {
    for (auto &span : paragraph.spans)
    {
      span.length = readU32(m_input);
      if (span.length > m_currentObjectHeader.size)
      {
        ZMF_DEBUG_MSG(("Incorrect span length, offset %ld\n", m_input->tell()));
        return;
      }

      skip(m_input, 4);

      uint32_t fontId = readU32(m_input);
      auto font = getFontByRefId(fontId);
      if (font)
        span.font = get(font);
      else
      {
        span.font = paragraph.style.font;
      }
    }
  }

  for (auto &paragraph : text.paragraphs)
  {
    for (auto &span : paragraph.spans)
    {
      auto size = span.length * 2;
      auto chars = readNBytes(m_input, size);

      appendCharacters(span.text, chars, size, "UTF-16LE");
    }
  }

  m_texts[get(m_currentObjectHeader.id)] = text;
}

void ZMF4Parser::readTextFrame()
{
  auto bbox = readBoundingBox();

  auto flags = readU8(m_input);

  auto alignment = VerticalAlignment::TOP;
  if (flags & 0x10)
    alignment = VerticalAlignment::MIDDLE;
  else if (flags & 0x20)
    alignment = VerticalAlignment::BOTTOM;

  boost::optional<Text> text;

  auto refs = readObjectRefs();

  for (const auto &ref : refs)
  {
    switch (ref.tag)
    {
    case 6:
    {
      text = getTextByRefId(ref.id);
    }
    break;
    default:
      ZMF_DEBUG_MSG(("unknown text frame ref tag %u\n", ref.tag));
      break;
    }
  }

  if (text)
  {
    m_collector.collectTextObject(get(text), bbox.topLeft(), bbox.width(), bbox.height(), alignment, bbox.rotation());
  }
}

void ZMF4Parser::readCurve()
{
  skip(m_input, 52);

  auto curves = readCurveComponents(std::bind(&ZMF4Parser::readPoint, this));

  m_collector.setStyle(readStyle());

  m_collector.collectPath(curves);
}

void ZMF4Parser::readRectangle()
{
  Curve curve;
  curve.points = readBoundingBox().points();
  curve.sectionTypes = std::vector<CurveType>(curve.points.size() - 1, CurveType::LINE);
  curve.closed = true;

  m_collector.setStyle(readStyle());

  m_collector.collectPath(curve);
}

void ZMF4Parser::readEllipse()
{
  auto bbox = readBoundingBox();

  float beginAngle = readFloat(m_input);
  float endAngle = readFloat(m_input);

  bool closed = !readU8(m_input);

  double rx = bbox.width() / 2;
  double ry = bbox.height() / 2;

  m_collector.setStyle(readStyle());

  if (ZMF_ALMOST_ZERO(beginAngle) && ZMF_ALMOST_ZERO(endAngle))
  {
    m_collector.collectEllipse(bbox.center(), rx, ry, bbox.rotation());
  }
  else
  {
    m_collector.collectArc(bbox.center(), rx, ry, beginAngle, endAngle, closed, bbox.rotation());
  }
}

void ZMF4Parser::readPolygon()
{
  auto bbox = readBoundingBox();

  double rx = bbox.width() / 2;
  double ry = bbox.height() / 2;

  uint32_t peaksCount = readU32(m_input);
  if (peaksCount == 0 || peaksCount > 99)
  {
    ZMF_DEBUG_MSG(("Incorrect peak count, offset %ld\n", m_input->tell()));
    return;
  }

  uint32_t pointsCount = readU32(m_input);
  const uint32_t endOffset = m_currentObjectHeader.refListStartOffset == 0
                             ? m_currentObjectHeader.nextObjectOffset
                             : m_currentObjectHeader.refListStartOffset
                             ;
  if (pointsCount == 0 || m_input->tell() + 8 >= endOffset || pointsCount > (endOffset - m_input->tell() - 8) / 12)
  {
    ZMF_DEBUG_MSG(("Incorrect peak point count, offset %ld\n", m_input->tell()));
    return;
  }

  skip(m_input, 8);

  Curve peakCurve;
  peakCurve.points.reserve(pointsCount);

  for (uint32_t i = 0; i < pointsCount; i++)
  {
    peakCurve.points.push_back(readUnscaledPoint());
  }

  readCurveSectionTypes(peakCurve.sectionTypes);

  m_collector.setStyle(readStyle());

  m_collector.collectPolygon(bbox.center(), rx, ry, peaksCount, peakCurve,
                             bbox.rotation(), bbox.mirrorHorizontal(), bbox.mirrorVertical());
}

void ZMF4Parser::readTable()
{
  Table table;

  auto bbox = readBoundingBox();

  table.width = bbox.width();
  table.height = bbox.height();

  table.topLeftPoint = bbox.topLeft();

  skip(m_input, 8);

  uint32_t rowCount = readU32(m_input);
  uint32_t colCount = readU32(m_input);

  if (rowCount < 1 || rowCount > 100 || colCount < 1 || colCount > 100)
  {
    ZMF_DEBUG_MSG(("Incorrect table size, offset %ld\n", m_input->tell()));
    return;
  }

  skip(m_input, 8);

  table.rows.resize(rowCount);
  table.columns.resize(colCount);

  for (auto &row : table.rows)
  {
    row.cells.resize(colCount);

    for (auto &cell : row.cells)
    {
      skip(m_input, 4);

      uint32_t fillRefId = readU32(m_input);
      cell.fill = getFillByRefId(fillRefId);

      uint32_t textId = readU32(m_input);
      auto text = getTextByRefId(textId);
      if (text)
      {
        cell.text = get(text);
      }

      uint32_t rightPenRefId = readU32(m_input);
      cell.rightBorder = getPenByRefId(rightPenRefId);

      uint32_t bottomPenRefId = readU32(m_input);
      cell.bottomBorder = getPenByRefId(bottomPenRefId);
    }
  }

  for (auto &row : table.rows)
  {
    skip(m_input, 4);

    uint32_t leftPenRefId = readU32(m_input);
    auto leftBorderPen = getPenByRefId(leftPenRefId);

    if (leftBorderPen)
    {
      for (auto &cell : row.cells)
      {
        if (!cell.leftBorder)
        {
          cell.leftBorder = leftBorderPen;
        }
      }
    }

    double relHeight = readFloat(m_input) / rowCount;
    row.height = table.height * relHeight;
  }

  for (size_t i = 0; i < table.columns.size(); i++)
  {
    auto &col = table.columns[i];
    skip(m_input, 4);

    uint32_t topPenRefId = readU32(m_input);
    auto topBorderPen = getPenByRefId(topPenRefId);

    if (topBorderPen)
    {
      for (auto &row : table.rows)
      {
        auto &cell = row.cells[i];

        if (!cell.topBorder)
        {
          cell.topBorder = topBorderPen;
        }
      }
    }

    double relWidth = readFloat(m_input) / colCount;
    col.width = table.width * relWidth;
  }

  auto tableStyle = readStyle();

  for (auto &row : table.rows)
  {
    for (auto &cell : row.cells)
    {
      if (tableStyle.fill && !cell.fill)
      {
        cell.fill = tableStyle.fill;
      }
    }
  }

  if (tableStyle.pen)
  {
    for (auto &cell : table.rows.front().cells)
    {
      cell.topBorder = tableStyle.pen;
    }
    for (auto &cell : table.rows.back().cells)
    {
      cell.bottomBorder = tableStyle.pen;
    }
    for (auto &row : table.rows)
    {
      row.cells.front().leftBorder = tableStyle.pen;
      row.cells.back().rightBorder = tableStyle.pen;
    }
  }

  m_collector.collectTable(table);
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
