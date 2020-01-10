/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ZMFCOLLECTOR_H_INCLUDED
#define ZMFCOLLECTOR_H_INCLUDED

#include <librevenge/librevenge.h>
#include "libzmf_utils.h"
#include "ZMFTypes.h"
#include <vector>

namespace libzmf
{

class ZMFCollector
{
  // disable copying
  ZMFCollector(const ZMFCollector &other) = delete;
  ZMFCollector &operator=(const ZMFCollector &other) = delete;

public:
  ZMFCollector(librevenge::RVNGDrawingInterface *painter);
  ~ZMFCollector();

  void startDocument();
  void endDocument();

  void startPage(const ZMFPageSettings &pageSettings);
  void endPage();

  void startLayer();
  void endLayer();

  void startGroup();
  void endGroup();

  void setStyle(const Style &style);

  void collectPath(const std::vector<Curve> &curves);
  void collectPath(const Curve &curve);

  void collectEllipse(const Point &c, double rx, double ry, double rotation);
  void collectArc(const Point &c, double rx, double ry, double beginAngle, double endAngle, bool closed, double rotation);

  void collectPolygon(const Point &c, double rx, double ry, uint32_t peaksCount, const Curve &peak,
                      double rotation, bool mirrorHorizontal, bool mirrorVertical);

  void collectTextObject(const Text &text, const Point &topLeft, double width, double height,
                         VerticalAlignment align, double rotation);

  void collectTable(const Table &table);

  void collectImage(const librevenge::RVNGBinaryData &image, const Point &topLeft, double width, double height,
                    double rotation, bool mirrorHorizontal, bool mirrorVertical);

private:
  double pageX(double canvasX);
  double pageY(double canvasY);

  void writeStyle(librevenge::RVNGPropertyList &propList, const Style &style, bool noFill = false);
  void writePen(librevenge::RVNGPropertyList &propList, const Pen &pen);
  void writeFill(librevenge::RVNGPropertyList &propList, const Fill &fill);
  void writeShadow(librevenge::RVNGPropertyList &propList, const Shadow &shadow);

  void collectText(const Text &text);
  void flushText(std::string &text);

  librevenge::RVNGDrawingInterface *m_painter;

  ZMFPageSettings m_pageSettings;

  bool m_isDocumentStarted;
  bool m_isPageStarted;
  bool m_isLayerStarted;

  Style m_style;
};

}

#endif // ZMFCOLLECTOR_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
