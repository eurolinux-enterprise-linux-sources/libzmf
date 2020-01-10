/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ZMFTYPES_H_INCLUDED
#define ZMFTYPES_H_INCLUDED

#include <vector>
#include <memory>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "libzmf_utils.h"

namespace libzmf
{

struct Point
{
  double x;
  double y;

  Point()
    : x(0.0), y(0.0)
  { }

  Point(double xVal, double yVal)
    : x(xVal), y(yVal)
  { }

  Point move(double dx, double dy) const;
  Point rotate(double rotation, const Point &center) const;

  double distance(const Point &p2) const;
};

bool operator==(const Point &lhs, const Point &rhs);
bool operator!=(const Point &lhs, const Point &rhs);

struct BoundingBox
{
  BoundingBox(const std::vector<Point> &points);

  const std::vector<Point> &points() const;

  double width() const;
  double height() const;

  Point center() const;
  Point topLeft() const;

  double rotation() const;

  int p1Quadrant() const;
  int p2Quadrant() const;

  bool mirrorHorizontal() const;
  bool mirrorVertical() const;

private:
  int quadrant(const Point &p) const;

  const std::vector<Point> m_points;
  double m_width;
  double m_height;
  Point m_center;
  double m_rotation;
  int m_p1Quadrant;
  int m_p2Quadrant;
  bool m_mirrorHorizontal;
  bool m_mirrorVertical;
};

enum class CurveType
{
  LINE,
  BEZIER_CURVE
};

struct Curve
{
  std::vector<Point> points;
  std::vector<CurveType> sectionTypes;
  bool closed;

  Curve()
    : points(), sectionTypes(), closed(false)
  {  }
};

struct Color
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  Color()
    : red(0), green(0), blue(0)
  {  }

  Color(uint8_t r, uint8_t g, uint8_t b)
    : red(r), green(g), blue(b)
  {  }

  librevenge::RVNGString toString() const;
};

enum class LineCapType
{
  BUTT,
  FLAT,
  ROUND,
  POINTED
};

enum class LineJoinType
{
  MITER,
  ROUND,
  BEVEL
};

struct Arrow
{
  std::vector<Curve> curves;
  double lineEndX;

  Arrow()
    : curves(), lineEndX(0.0)
  { }
};

typedef std::shared_ptr<Arrow> ArrowPtr;

struct Pen
{
  Color color;
  double width;
  LineCapType lineCapType;
  LineJoinType lineJoinType;
  std::vector<double> dashPattern;
  double dashDistance;
  ArrowPtr startArrow;
  ArrowPtr endArrow;
  bool isInvisible;

  Pen()
    : color(), width(0.0),
      lineCapType(LineCapType::BUTT), lineJoinType(LineJoinType::MITER),
      dashPattern(), dashDistance(0),
      startArrow(), endArrow(),
      isInvisible(false)
  {  }

  Pen(Color c)
    : color(c),
      width(0.0),
      lineCapType(LineCapType::BUTT), lineJoinType(LineJoinType::MITER),
      dashPattern(), dashDistance(0),
      startArrow(), endArrow(),
      isInvisible(false)
  {  }
};

struct GradientStop
{
  Color color;
  double offset;

  GradientStop()
    : color(), offset(0.0)
  { }
};

enum class GradientType
{
  LINEAR,
  RADIAL,
  CONICAL,
  CROSS,
  RECTANGULAR,
  FLEXIBLE
};

struct Gradient
{
  GradientType type;
  std::vector<GradientStop> stops;
  double angle;
  Point center;

  Gradient()
    : type(), stops(), angle(0.0), center(0.5, 0.5)
  { }
};

struct Image
{
  uint32_t width;
  uint32_t height;
  librevenge::RVNGBinaryData data;

  Image()
    : width(0), height(0), data()
  { }

  Image(uint32_t w, uint32_t h, librevenge::RVNGBinaryData d)
    : width(w), height(h), data(d)
  { }
};

struct ImageFill
{
  Image image;
  bool tile;
  double tileWidth;
  double tileHeight;

  ImageFill()
    : image(), tile(false), tileWidth(0.0), tileHeight(0.0)
  { }
};

typedef boost::variant<Color, Gradient, ImageFill> Fill;

struct Transparency
{
  Color color;

  double opacity() const
  {
    return 1.0 - color.red / 255.0;
  }

  Transparency()
    : color()
  { }
};

struct Shadow
{
  Point offset;
  double angle;
  double opacity;
  Color color;

  Shadow()
    : offset(), angle(0.0), opacity(1.0), color()
  { }
};

struct Style
{
  boost::optional<Pen> pen;
  boost::optional<Fill> fill;
  boost::optional<Transparency> transparency;
  boost::optional<Shadow> shadow;

  Style()
    : pen(), fill(), transparency(), shadow()
  { }
};

struct Font
{
  librevenge::RVNGString name;
  double size;
  bool isBold;
  bool isItalic;
  boost::optional<Fill> fill;
  boost::optional<Pen> outline;

  Font()
    : name("Arial"), size(24.0), isBold(false), isItalic(false),
      fill(Color(0, 0, 0)), outline()
  { }
};

struct Span
{
  librevenge::RVNGString text;
  uint32_t length;
  Font font;

  Span()
    : text(), length(0), font()
  { }
};

enum class HorizontalAlignment
{
  LEFT,
  RIGHT,
  BLOCK,
  CENTER,
  FULL
};

enum class VerticalAlignment
{
  TOP,
  MIDDLE,
  BOTTOM
};

struct ParagraphStyle
{
  double lineSpacing;
  HorizontalAlignment alignment;
  Font font;

  ParagraphStyle()
    : lineSpacing(1.2), alignment(HorizontalAlignment::LEFT), font()
  { }
};

struct Paragraph
{
  std::vector<Span> spans;
  ParagraphStyle style;

  Paragraph()
    : spans(), style()
  { }
};

struct Text
{
  std::vector<Paragraph> paragraphs;

  Text()
    : paragraphs()
  { }
};

struct Cell
{
  Text text;
  boost::optional<Fill> fill;
  boost::optional<Pen> leftBorder;
  boost::optional<Pen> rightBorder;
  boost::optional<Pen> topBorder;
  boost::optional<Pen> bottomBorder;

  Cell()
    : text(), fill(),
      leftBorder(), rightBorder(), topBorder(), bottomBorder()
  { }
};

struct Row
{
  std::vector<Cell> cells;
  double height;

  Row()
    : cells(), height(0.0)
  { }
};

struct Column
{
  double width;

  Column()
    : width(0.0)
  { }
};

struct Table
{
  std::vector<Row> rows;
  std::vector<Column> columns;
  double width;
  double height;
  Point topLeftPoint;

  Table()
    : rows(), columns(), width(0.0), height(0.0), topLeftPoint()
  { }
};

struct ZMFPageSettings
{
  double width;
  double height;
  double leftOffset;
  double topOffset;
  Color color;

  ZMFPageSettings()
    : width(0.0), height(0.0), leftOffset(0.0), topOffset(0.0), color()
  { }
  ZMFPageSettings(double w, double h, double left, double top, Color c = Color(255, 255, 255))
    : width(w), height(h), leftOffset(left), topOffset(top), color(c)
  { }
};

}

#endif // ZMFTYPES_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
