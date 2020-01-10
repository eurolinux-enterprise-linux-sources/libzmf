/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ZMFCollector.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <boost/math/constants/constants.hpp>
#include <boost/variant.hpp>

namespace libzmf
{

namespace
{

Point calculateEllipsePoint(const Point &c, double rx, double ry, double angle)
{
  return Point(c.x + rx * std::cos(angle), c.y + ry * std::sin(angle));
}

void writeBorder(librevenge::RVNGPropertyList &propList, const char *const name, const Pen &pen)
{
  if (pen.isInvisible)
    return;

  librevenge::RVNGString border;

  border.sprintf("%fin", pen.width);

  border.append(" solid");

  border.append(" ");
  border.append(pen.color.toString());

  propList.insert(name, border);
}

librevenge::RVNGPropertyListVector createPath(const std::vector<Curve> &curves,
    double leftOffset = 0.0, double topOffset = 0.0)
{
  librevenge::RVNGPropertyListVector path;

  for (const auto &curve : curves)
  {
    if (curve.points.size() < 2)
      continue;

    {
      librevenge::RVNGPropertyList pathPart;
      pathPart.insert("librevenge:path-action", "M");
      pathPart.insert("svg:x", curve.points[0].x - leftOffset);
      pathPart.insert("svg:y", curve.points[0].y - topOffset);

      path.append(pathPart);
    }

    size_t i = 1;

    for (const auto &sectionType : curve.sectionTypes)
    {
      librevenge::RVNGPropertyList pathPart;

      switch (sectionType)
      {
      default:
      case CurveType::LINE:
        if (i >= curve.points.size())
        {
          ZMF_DEBUG_MSG(("Unexpected end of curve points\n"));
          break;
        }
        pathPart.insert("librevenge:path-action", "L");
        pathPart.insert("svg:x", curve.points[i].x - leftOffset);
        pathPart.insert("svg:y", curve.points[i].y - topOffset);
        i++;
        break;
      case CurveType::BEZIER_CURVE:
        if (i + 2 >= curve.points.size())
        {
          ZMF_DEBUG_MSG(("Unexpected end of curve points\n"));
          break;
        }
        pathPart.insert("librevenge:path-action", "C");
        pathPart.insert("svg:x1", curve.points[i].x - leftOffset);
        pathPart.insert("svg:y1", curve.points[i].y - topOffset);
        pathPart.insert("svg:x2", curve.points[i + 1].x - leftOffset);
        pathPart.insert("svg:y2", curve.points[i + 1].y - topOffset);
        pathPart.insert("svg:x", curve.points[i + 2].x - leftOffset);
        pathPart.insert("svg:y", curve.points[i + 2].y - topOffset);
        i += 3;
        break;
      }

      path.append(pathPart);
    }

    if (curve.closed)
    {
      librevenge::RVNGPropertyList pathPart;
      pathPart.insert("librevenge:path-action", "Z");

      path.append(pathPart);
    }
  }

  return path;
}

librevenge::RVNGString getPathStr(const librevenge::RVNGPropertyListVector &path)
{
  librevenge::RVNGString s("");

  for (unsigned i = 0; i < path.count(); ++i)
  {
    if (!path[i]["librevenge:path-action"])
      continue;

    std::string action = path[i]["librevenge:path-action"]->getStr().cstr();
    bool coordOk = path[i]["svg:x"] && path[i]["svg:y"];
    bool coord1Ok = coordOk && path[i]["svg:x1"] && path[i]["svg:y1"];
    bool coord2Ok = coord1Ok && path[i]["svg:x2"] && path[i]["svg:y2"];

    librevenge::RVNGString sElement;

    switch (action[0])
    {
    case 'M':
    case 'L':
      if (!coordOk)
      {
        ZMF_DEBUG_MSG(("Incorrect path coordinates\n"));
        break;
      }
      sElement.sprintf("%c%lf %lf ", action[0], path[i]["svg:x"]->getDouble(), path[i]["svg:y"]->getDouble());
      s.append(sElement);
      break;
    case 'C':
      if (!coord2Ok)
      {
        ZMF_DEBUG_MSG(("Incorrect path coordinates\n"));
        break;
      }
      sElement.sprintf("C%lf %lf %lf %lf %lf %lf ",
                       path[i]["svg:x1"]->getDouble(), path[i]["svg:y1"]->getDouble(),
                       path[i]["svg:x2"]->getDouble(), path[i]["svg:y2"]->getDouble(),
                       path[i]["svg:x"]->getDouble(), path[i]["svg:y"]->getDouble());
      s.append(sElement);
      break;
    case 'Z':
      s.append("Z ");
      break;
    default:
      ZMF_DEBUG_MSG(("Unknown path-action %s\n", action.c_str()));
    }
  }

  return s;
}

void writeArrow(librevenge::RVNGPropertyList &propList, const char *const name, Arrow arrow, double penWidth)
{
  using namespace boost::math::double_constants;

  Point lineEnd(arrow.lineEndX, 0);

  lineEnd = lineEnd.rotate(half_pi, Point(0, 0));

  double dist = 1;

  for (auto &curve : arrow.curves)
  {
    auto minmaxY = std::minmax_element(curve.points.begin(), curve.points.end(), [](const Point &p1, const Point &p2)
    {
      return p1.y < p2.y;
    });

    dist = std::max(dist, std::abs(minmaxY.first->y - minmaxY.second->y));

    std::transform(curve.points.begin(), curve.points.end(), curve.points.begin(), [&lineEnd](const Point &p)
    {
      return p.rotate(half_pi, Point(0, 0)).move(0, -lineEnd.y);
    });
  }

  librevenge::RVNGString propName;

  propName.sprintf("draw:marker-%s-viewbox", name);
  propList.insert(propName.cstr(), "-10 -10 10 10");
  propName.sprintf("draw:marker-%s-path", name);
  propList.insert(propName.cstr(), getPathStr(createPath(arrow.curves)));
  propName.sprintf("draw:marker-%s-width", name);
  propList.insert(propName.cstr(), penWidth * dist);
  propName.sprintf("draw:marker-%s-center", name);
  propList.insert(propName.cstr(), true);
}

class FillWriter : public boost::static_visitor<void>
{
public:
  explicit FillWriter(librevenge::RVNGPropertyList &propList,
                      const boost::optional<Transparency> &transparency)
    : m_propList(propList)
    , m_transparency(transparency)
  { }

  void operator()(const Color &color)
  {
    m_propList.insert("draw:fill", "solid");
    m_propList.insert("draw:fill-color", color.toString());

    if (m_transparency)
    {
      m_propList.insert("draw:opacity", get(m_transparency).opacity(), librevenge::RVNG_PERCENT);
    }
  }

  void operator()(const Gradient &gradient)
  {
    using namespace boost::math::double_constants;

    if (gradient.stops.size() < 2)
      return;

    m_propList.insert("draw:fill", "gradient");

    auto stops = gradient.stops;
    std::sort(stops.begin(), stops.end(),
              [&gradient](const GradientStop &s1, const GradientStop &s2)
    {
      return gradient.type == GradientType::LINEAR ? s1.offset < s2.offset : s1.offset > s2.offset;
    });
    if (gradient.type != GradientType::LINEAR)
    {
      for (auto &stop : stops)
      {
        stop.offset = 1.0 - stop.offset;
      }
    }

    librevenge::RVNGPropertyListVector gradientVector;
    for (const auto &stop : stops)
    {
      librevenge::RVNGPropertyList grad;
      grad.insert("svg:offset", stop.offset, librevenge::RVNG_PERCENT);
      grad.insert("svg:stop-color", stop.color.toString());
      grad.insert("svg:stop-opacity", m_transparency ? get(m_transparency).opacity() : 1.0, librevenge::RVNG_PERCENT);
      gradientVector.append(grad);
    }

    switch (gradient.type)
    {
    default:
    case GradientType::LINEAR:
      m_propList.insert("draw:style", "linear");
      m_propList.insert("draw:angle", rad2deg(gradient.angle + half_pi));
      m_propList.insert("svg:linearGradient", gradientVector);
      break;
    case GradientType::RADIAL:
      m_propList.insert("draw:style", "radial");
      m_propList.insert("draw:cx", gradient.center.x, librevenge::RVNG_PERCENT);
      m_propList.insert("draw:cy", gradient.center.y, librevenge::RVNG_PERCENT);
      m_propList.insert("draw:border", 0.25 - gradient.center.distance(Point(0.5, 0.5)), librevenge::RVNG_PERCENT);
      m_propList.insert("svg:radialGradient", gradientVector);
      break;
    }
  }

  void operator()(const ImageFill &imageFill)
  {
    m_propList.insert("draw:fill", "bitmap");

    m_propList.insert("draw:fill-image", imageFill.image.data);
    m_propList.insert("librevenge:mime-type", "image/png");

    if (imageFill.tile)
    {
      m_propList.insert("style:repeat", "repeat");
      m_propList.insert("draw:fill-image-width", imageFill.tileWidth, librevenge::RVNG_INCH);
      m_propList.insert("draw:fill-image-height", imageFill.tileHeight, librevenge::RVNG_INCH);
      m_propList.insert("draw:fill-image-ref-point", "top-left");
    }
    else
    {
      m_propList.insert("style:repeat", "stretch");
    }

    if (m_transparency)
    {
      m_propList.insert("draw:opacity", get(m_transparency).opacity(), librevenge::RVNG_PERCENT);
    }
  }

private:
  librevenge::RVNGPropertyList &m_propList;

  const boost::optional<Transparency> &m_transparency;
};

}

ZMFCollector::ZMFCollector(librevenge::RVNGDrawingInterface *painter)
  : m_painter(painter)
  , m_pageSettings()
  , m_isDocumentStarted(false)
  , m_isPageStarted(false)
  , m_isLayerStarted(false)
  , m_style()
{
}

ZMFCollector::~ZMFCollector()
{
  if (m_isDocumentStarted)
  {
    endDocument();
  }
}

void ZMFCollector::startDocument()
{
  if (m_isDocumentStarted)
    return;

  librevenge::RVNGPropertyList propList;

  m_painter->startDocument(propList);

  m_isDocumentStarted = true;
}

void ZMFCollector::endDocument()
{
  if (!m_isDocumentStarted)
    return;

  if (m_isPageStarted)
    endPage();

  m_painter->endDocument();

  m_isDocumentStarted = false;
}

void ZMFCollector::startPage(const ZMFPageSettings &pageSettings)
{
  if (m_isPageStarted)
    return;

  if (m_isLayerStarted)
    endLayer();

  librevenge::RVNGPropertyList propList;

  propList.insert("svg:width", pageSettings.width);
  propList.insert("svg:height", pageSettings.height);

  propList.insert("draw:fill", "solid");
  propList.insert("draw:fill-color", pageSettings.color.toString());

  m_painter->startPage(propList);

  m_pageSettings = pageSettings;

  m_isPageStarted = true;
}

void ZMFCollector::endPage()
{
  if (!m_isPageStarted)
    return;

  m_painter->endPage();

  m_isPageStarted = false;
}

void ZMFCollector::startLayer()
{
  if (m_isLayerStarted)
    return;

  librevenge::RVNGPropertyList propList;

  m_painter->startLayer(propList);

  m_isLayerStarted = true;
}

void ZMFCollector::endLayer()
{
  if (!m_isLayerStarted)
    return;

  m_painter->endLayer();

  m_isLayerStarted = false;
}

void ZMFCollector::startGroup()
{
  librevenge::RVNGPropertyList propList;

  m_painter->openGroup(propList);
}

void ZMFCollector::endGroup()
{
  m_painter->closeGroup();
}

void ZMFCollector::setStyle(const Style &style)
{
  m_style = style;
}

void ZMFCollector::collectPath(const std::vector<Curve> &curves)
{
  librevenge::RVNGPropertyList pathPropList;

  writeStyle(pathPropList, m_style,
             !std::any_of(curves.begin(), curves.end(), [](const Curve &c)
  {
    return c.closed;
  }));
  m_painter->setStyle(pathPropList);
  pathPropList.clear();

  auto path = createPath(curves, m_pageSettings.leftOffset, m_pageSettings.topOffset);

  pathPropList.insert("svg:d", path);

  m_painter->drawPath(pathPropList);
}

void ZMFCollector::collectPath(const Curve &curve)
{
  collectPath(std::vector<Curve> { curve });
}

void ZMFCollector::collectEllipse(const Point &c, double rx, double ry, double rotation)
{
  librevenge::RVNGPropertyList ellipse;

  writeStyle(ellipse, m_style);
  m_painter->setStyle(ellipse);
  ellipse.clear();

  ellipse.insert("svg:cx", pageX(c.x));
  ellipse.insert("svg:cy", pageY(c.y));
  ellipse.insert("svg:rx", rx);
  ellipse.insert("svg:ry", ry);
  if (!ZMF_ALMOST_ZERO(rotation))
  {
    ellipse.insert("librevenge:rotate", -rad2deg(rotation));
  }

  m_painter->drawEllipse(ellipse);
}

void ZMFCollector::collectArc(const Point &c, double rx, double ry, double beginAngle, double endAngle, bool closed, double rotation)
{
  using namespace boost::math::double_constants;

  librevenge::RVNGPropertyList pathPropList;

  writeStyle(pathPropList, m_style, !closed);
  m_painter->setStyle(pathPropList);
  pathPropList.clear();

  auto beginPoint = calculateEllipsePoint(c, rx, ry, beginAngle);
  auto endPoint = calculateEllipsePoint(c, rx, ry, endAngle);

  if (!ZMF_ALMOST_ZERO(rotation))
  {
    beginPoint = beginPoint.rotate(rotation, c);
    endPoint = endPoint.rotate(rotation, c);
  }

  double angleDiff = std::fabs(endAngle - beginAngle);
  bool largeArc = (beginAngle < endAngle && angleDiff > pi) ||
                  (beginAngle > endAngle && angleDiff < pi);

  librevenge::RVNGPropertyListVector path;

  {
    librevenge::RVNGPropertyList pathStart;
    pathStart.insert("librevenge:path-action", "M");
    pathStart.insert("svg:x", pageX(beginPoint.x));
    pathStart.insert("svg:y", pageY(beginPoint.y));

    path.append(pathStart);
  }

  {
    librevenge::RVNGPropertyList arc;
    arc.insert("librevenge:path-action", "A");
    arc.insert("svg:rx", rx);
    arc.insert("svg:ry", ry);
    arc.insert("librevenge:large-arc", largeArc ? 1 : 0);
    arc.insert("librevenge:sweep", 1);
    arc.insert("svg:x", pageX(endPoint.x));
    arc.insert("svg:y", pageY(endPoint.y));

    path.append(arc);
  }

  if (closed)
  {
    librevenge::RVNGPropertyList pathEndLine;
    pathEndLine.insert("librevenge:path-action", "L");
    pathEndLine.insert("svg:x", pageX(c.x));
    pathEndLine.insert("svg:y", pageY(c.y));

    path.append(pathEndLine);

    librevenge::RVNGPropertyList pathEnd;
    pathEnd.insert("librevenge:path-action", "Z");

    path.append(pathEnd);
  }

  pathPropList.insert("svg:d", path);

  m_painter->drawPath(pathPropList);
}

void ZMFCollector::collectPolygon(const Point &c, double rx, double ry, uint32_t peaksCount, const Curve &peak,
                                  double rotation, bool mirrorHorizontal, bool mirrorVertical)
{
  using namespace boost::math::double_constants;

  if (peak.points.size() < 2)
  {
    return;
  }

  const double peakAngle = two_pi / peaksCount;

  // create a single side of the polygon in an unit square; the
  // center of the (future) polygon is (0, 0)
  std::vector<Point> side;
  side.reserve(peak.points.size());
  std::transform(peak.points.begin(), peak.points.end(), std::back_inserter(side), [peakAngle](const Point &p)
  {
    return calculateEllipsePoint(Point(0, 0), p.y, p.y, p.x * peakAngle);
  });

  Curve polygonCurve;
  polygonCurve.points.reserve(peak.points.size() * peaksCount);
  polygonCurve.sectionTypes.reserve(peak.sectionTypes.size() * peaksCount);

  // generate complete polygon
  for (uint32_t i = 0; i < peaksCount; ++i)
  {
    std::transform(side.begin() + (i == 0 ? 0 : 1), side.end(), std::back_inserter(polygonCurve.points), [i, peakAngle](Point p)
    {
      p =  p.rotate(i * peakAngle, Point(0, 0));
      return p;
    });
    std::copy(peak.sectionTypes.begin(), peak.sectionTypes.end(), std::back_inserter(polygonCurve.sectionTypes));
  }

  // fit the polygon into the bbox and mirror
  for (auto &p: polygonCurve.points)
  {
    p.x = p.x * rx;
    p.y = p.y * ry;
    p = p.move(c.x, c.y);

    p.y = -p.y;
    p = p.move(0, 2 * c.y);

    if (mirrorHorizontal)
    {
      p.x = -p.x;
      p = p.move(2 * c.x, 0);
    }
    if (mirrorVertical)
    {
      p.y = -p.y;
      p = p.move(0, 2 * c.y);
    }

    p = p.rotate(rotation, c);
  }

  polygonCurve.closed = true;
  collectPath(polygonCurve);
}

void ZMFCollector::collectTextObject(const Text &text, const Point &topLeft, double width, double height,
                                     VerticalAlignment align, double rotation)
{
  librevenge::RVNGPropertyList textObjPropList;

  textObjPropList.insert("svg:x", pageX(topLeft.x));
  textObjPropList.insert("svg:y", pageY(topLeft.y));
  textObjPropList.insert("svg:width", width);
  textObjPropList.insert("svg:height", height);

  switch (align)
  {
  case VerticalAlignment::TOP:
    textObjPropList.insert("draw:textarea-vertical-align", "top");
    break;
  case VerticalAlignment::MIDDLE:
    textObjPropList.insert("draw:textarea-vertical-align", "middle");
    break;
  case VerticalAlignment::BOTTOM:
    textObjPropList.insert("draw:textarea-vertical-align", "bottom");
    break;
  }
  if (!ZMF_ALMOST_ZERO(rotation))
  {
    textObjPropList.insert("librevenge:rotate", rad2deg(rotation));
  }

  m_painter->startTextObject(textObjPropList);

  collectText(text);

  m_painter->endTextObject();
}

void ZMFCollector::collectText(const Text &text)
{
  for (auto &paragraph : text.paragraphs)
  {
    librevenge::RVNGPropertyList paragraphPropList;

    paragraphPropList.insert("fo:line-height", paragraph.style.lineSpacing, librevenge::RVNG_PERCENT);

    switch (paragraph.style.alignment)
    {
    case HorizontalAlignment::LEFT:
      paragraphPropList.insert("fo:text-align", "left");
      break;
    case HorizontalAlignment::RIGHT:
      paragraphPropList.insert("fo:text-align", "end");
      break;
    case HorizontalAlignment::CENTER:
      paragraphPropList.insert("fo:text-align", "center");
      break;
    case HorizontalAlignment::BLOCK:
    case HorizontalAlignment::FULL:
      paragraphPropList.insert("fo:text-align", "justify");
      break;
    }

    m_painter->openParagraph(paragraphPropList);

    for (auto &span : paragraph.spans)
    {
      librevenge::RVNGPropertyList spanPropList;

      spanPropList.insert("style:font-name", span.font.name);
      spanPropList.insert("fo:font-size", span.font.size, librevenge::RVNG_POINT);
      spanPropList.insert("fo:font-weight", span.font.isBold ? "bold" : "normal");
      spanPropList.insert("fo:font-style", span.font.isItalic ? "italic" : "normal");
      spanPropList.insert("style:text-outline", static_cast<bool>(span.font.outline));

      if (span.font.fill && get(span.font.fill).type() == typeid(Color))
      {
        auto color = boost::get<Color>(get(span.font.fill));
        spanPropList.insert("fo:color", color.toString());
      }

      m_painter->openSpan(spanPropList);

      bool wasSpace = false;
      std::string curText;

      librevenge::RVNGString::Iter iter(span.text);
      iter.rewind();
      while (iter.next())
      {
        const char *const utf8Char = iter();
        switch (utf8Char[0])
        {
        // looks like Zoner Draw doesn't allow tabs,
        // and \r (without \n) can be only at the end of paragraph
        case '\r':
        case '\n':
          break;
        case ' ':
          if (wasSpace)
          {
            flushText(curText);
            m_painter->insertSpace();
          }
          else
          {
            wasSpace = true;
            curText.push_back(' ');
          }
          break;
        default:
          wasSpace = false;
          curText.append(utf8Char);
          break;
        }
      }

      flushText(curText);

      m_painter->closeSpan();
    }

    m_painter->closeParagraph();
  }
}

void ZMFCollector::flushText(std::string &text)
{
  if (!text.empty())
  {
    m_painter->insertText(librevenge::RVNGString(text.c_str()));
    text.clear();
  }
}

void ZMFCollector::collectTable(const Table &table)
{
  librevenge::RVNGPropertyList tablePropList;

  tablePropList.insert("svg:x", pageX(table.topLeftPoint.x));
  tablePropList.insert("svg:y", pageY(table.topLeftPoint.y));
  tablePropList.insert("svg:width", table.width);
  tablePropList.insert("svg:height", table.height);

  librevenge::RVNGPropertyListVector columnSizes;

  for (const auto &col : table.columns)
  {
    librevenge::RVNGPropertyList columnPropList;
    columnPropList.insert("style:column-width", col.width);
    columnSizes.append(columnPropList);
  }

  tablePropList.insert("librevenge:table-columns", columnSizes);

  m_painter->startTableObject(tablePropList);

  for (const auto &row : table.rows)
  {
    librevenge::RVNGPropertyList rowPropList;
    rowPropList.insert("style:row-height", row.height);
    m_painter->openTableRow(rowPropList);

    for (const Cell &cell : row.cells)
    {
      librevenge::RVNGPropertyList cellPropList;

      if (cell.fill && get(cell.fill).type() == typeid(Color))
      {
        auto backgroundColor = boost::get<Color>(get(cell.fill));
        cellPropList.insert("fo:background-color", backgroundColor.toString());
      }

      cellPropList.insert("draw:textarea-vertical-align", "middle");

      if (cell.leftBorder)
      {
        writeBorder(cellPropList, "fo:border-left", get(cell.leftBorder));
      }
      if (cell.rightBorder)
      {
        writeBorder(cellPropList, "fo:border-right", get(cell.rightBorder));
      }
      if (cell.topBorder)
      {
        writeBorder(cellPropList, "fo:border-top", get(cell.topBorder));
      }
      if (cell.bottomBorder)
      {
        writeBorder(cellPropList, "fo:border-bottom", get(cell.bottomBorder));
      }

      m_painter->openTableCell(cellPropList);

      collectText(cell.text);

      m_painter->closeTableCell();
    }

    m_painter->closeTableRow();
  }

  m_painter->endTableObject();
}

void ZMFCollector::collectImage(const librevenge::RVNGBinaryData &image, const Point &topLeft, double width, double height,
                                double rotation, bool mirrorHorizontal, bool mirrorVertical)
{
  librevenge::RVNGPropertyList propList;

  writeStyle(propList, m_style);

  if (m_style.transparency)
  {
    propList.insert("draw:opacity", get(m_style.transparency).opacity(), librevenge::RVNG_PERCENT);
  }

  m_painter->setStyle(propList);
  propList.clear();

  propList.insert("svg:x", pageX(topLeft.x));
  propList.insert("svg:y", pageY(topLeft.y));
  propList.insert("svg:width", width);
  propList.insert("svg:height", height);
  if (!ZMF_ALMOST_ZERO(rotation))
  {
    propList.insert("librevenge:rotate", rad2deg(rotation));
  }
  propList.insert("draw:mirror-vertical", mirrorVertical);
  propList.insert("draw:mirror-horizontal", mirrorHorizontal);
  propList.insert("librevenge:mime-type", "image/png");
  propList.insert("office:binary-data", image);

  m_painter->drawGraphicObject(propList);
}

double ZMFCollector::pageX(double canvasX)
{
  return canvasX - m_pageSettings.leftOffset;
}

double ZMFCollector::pageY(double canvasY)
{
  return canvasY - m_pageSettings.topOffset;
}

// noFill is used to ignore Fill object even if it exists
// some implementations (such as svg in web browsers) try to fill not closed paths if fill is specified
void ZMFCollector::writeStyle(librevenge::RVNGPropertyList &propList, const Style &style, bool noFill)
{
  propList.insert("draw:stroke", "none");
  propList.insert("draw:fill", "none");

  if (style.pen)
  {
    writePen(propList, get(style.pen));
  }

  if (style.fill && !noFill)
  {
    writeFill(propList, get(style.fill));
  }

  if (style.shadow)
  {
    writeShadow(propList, get(style.shadow));
  }
}

void ZMFCollector::writePen(librevenge::RVNGPropertyList &propList, const Pen &pen)
{
  propList.insert("svg:stroke-color", pen.color.toString());
  if (!ZMF_ALMOST_ZERO(pen.width))
    propList.insert("svg:stroke-width", pen.width);

  if (pen.dashPattern.size() > 0)
  {
    double dots1 = pen.dashPattern[0];
    double dots2 = pen.dashPattern[0];
    double dist = pen.dashDistance;
    if (pen.dashPattern.size() >= 3)
    {
      dist = pen.dashPattern[1];
      dots2 = pen.dashPattern[2];
    }

    propList.insert("draw:stroke", "dash");
    propList.insert("draw:dots1", 1);
    propList.insert("draw:dots1-length", dots1, librevenge::RVNG_PERCENT);
    propList.insert("draw:dots2", 1);
    propList.insert("draw:dots2-length", dots2, librevenge::RVNG_PERCENT);
    propList.insert("draw:distance", dist, librevenge::RVNG_PERCENT);
  }
  else
  {
    propList.insert("draw:stroke", "solid");
  }

  switch (pen.lineCapType)
  {
  default:
  case LineCapType::BUTT:
    propList.insert("svg:stroke-linecap", "butt");
    break;
  case LineCapType::ROUND:
    propList.insert("svg:stroke-linecap", "round");
    break;
  case LineCapType::FLAT:
    propList.insert("svg:stroke-linecap", "square");
    break;
  }

  switch (pen.lineJoinType)
  {
  default:
  case LineJoinType::BEVEL:
    propList.insert("svg:stroke-linejoin", "bevel");
    break;
  case LineJoinType::MITER:
    propList.insert("svg:stroke-linejoin", "miter");
    break;
  case LineJoinType::ROUND:
    propList.insert("svg:stroke-linejoin", "round");
    break;
  }

  if (m_style.transparency)
  {
    propList.insert("svg:stroke-opacity", get(m_style.transparency).opacity(), librevenge::RVNG_PERCENT);
  }

  if (pen.startArrow)
  {
    writeArrow(propList, "start", *pen.startArrow, pen.width);
  }
  if (pen.endArrow)
  {
    writeArrow(propList, "end", *pen.endArrow, pen.width);
  }
}

void ZMFCollector::writeFill(librevenge::RVNGPropertyList &propList, const Fill &fill)
{
  FillWriter fillWriter(propList, m_style.transparency);
  boost::apply_visitor(fillWriter, fill);

  propList.insert("svg:fill-rule", "evenodd");
}

void ZMFCollector::writeShadow(librevenge::RVNGPropertyList &propList, const Shadow &shadow)
{
  propList.insert("draw:shadow", "visible");
  propList.insert("draw:shadow-color", shadow.color.toString());
  propList.insert("draw:shadow-opacity", shadow.opacity, librevenge::RVNG_PERCENT);
  propList.insert("draw:shadow-offset-x", shadow.offset.x);
  propList.insert("draw:shadow-offset-y", shadow.offset.y);
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
