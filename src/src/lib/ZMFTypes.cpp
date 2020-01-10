/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ZMFTypes.h"

#include <boost/math/constants/constants.hpp>

namespace libzmf
{

bool operator==(const Point &lhs, const Point &rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const Point &lhs, const Point &rhs)
{
  return !(lhs == rhs);
}

Point Point::move(double dx, double dy) const
{
  return Point(x + dx, y + dy);
}

Point Point::rotate(double rotation, const Point &center) const
{
  double rotatedX = (x - center.x) * std::cos(rotation) - (y - center.y) * std::sin(rotation) + center.x;
  double rotatedY = (y - center.y) * std::cos(rotation) + (x - center.x) * std::sin(rotation) + center.y;
  return Point(rotatedX, rotatedY);
}

double Point::distance(const Point &p2) const
{
  return std::hypot(p2.x - x, p2.y - y);
}


BoundingBox::BoundingBox(const std::vector<Point> &points_)
  : m_points(points_)
  , m_width(0.0)
  , m_height(0.0)
  , m_center()
  , m_rotation(0)
  , m_p1Quadrant(0)
  , m_p2Quadrant(0)
  , m_mirrorHorizontal(false)
  , m_mirrorVertical(false)
{
  using namespace boost::math::double_constants;

  if (m_points.size() != 4)
    throw GenericException();

  // rectangle center is middle point of diagonal
  m_center = Point((m_points[0].x + m_points[2].x) / 2.0, (m_points[0].y + m_points[2].y) / 2.0);

  m_rotation = std::atan2(m_points[1].y - m_points[0].y, m_points[1].x - m_points[0].x);
  if (m_rotation < 0)
    m_rotation += two_pi;

  std::vector<Point> originalPoints;
  if (ZMF_ALMOST_ZERO(m_rotation))
  {
    originalPoints = m_points;
  }
  else
  {
    originalPoints.reserve(m_points.size());
    for (auto p : m_points)
    {
      originalPoints.push_back(p.rotate(-m_rotation, m_center));
    }
  }

  double dist1 = m_points[0].distance(m_points[1]);
  double dist2 = m_points[0].distance(m_points[3]);

  if (std::abs(originalPoints[0].x - originalPoints[1].x) > std::abs(originalPoints[0].x - originalPoints[3].x))
  {
    m_width = dist1;
    m_height = dist2;
  }
  else
  {
    m_width = dist2;
    m_height = dist1;
  }

  m_p1Quadrant = quadrant(originalPoints[0]);
  m_p2Quadrant = quadrant(originalPoints[1]);

  if (p1Quadrant() == 1 || p1Quadrant() == 4)
    m_rotation -= pi;
  if (m_rotation < 0)
    m_rotation += two_pi;

  m_mirrorHorizontal = m_p1Quadrant == 1 || m_p1Quadrant == 4;
  m_mirrorVertical = m_p1Quadrant == 3 || m_p1Quadrant == 4;
}

const std::vector<Point> &BoundingBox::points() const
{
  return m_points;
}

double BoundingBox::width() const
{
  return m_width;
}

double BoundingBox::height() const
{
  return m_height;
}

Point BoundingBox::center() const
{
  return m_center;
}

Point BoundingBox::topLeft() const
{
  return center().move(-width() / 2.0, -height() / 2.0);
}

double BoundingBox::rotation() const
{
  return m_rotation;
}

int BoundingBox::p1Quadrant() const
{
  return m_p1Quadrant;
}

int BoundingBox::p2Quadrant() const
{
  return m_p2Quadrant;
}

bool BoundingBox::mirrorHorizontal() const
{
  return m_mirrorHorizontal;
}

bool BoundingBox::mirrorVertical() const
{
  return m_mirrorVertical;
}

int BoundingBox::quadrant(const Point &p) const
{
  if (p.x > m_center.x)
  {
    if (p.y < m_center.y)
      return 1;
    else
      return 4;
  }
  else
  {
    if (p.y < m_center.y)
      return 2;
    else
      return 3;
  }
}


librevenge::RVNGString Color::toString() const
{
  librevenge::RVNGString colorStr;
  colorStr.sprintf("#%.2x%.2x%.2x", red, green, blue);
  return colorStr;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
