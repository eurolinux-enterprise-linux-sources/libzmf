/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <algorithm>
#include <vector>

#include <boost/math/constants/constants.hpp>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <librevenge-stream/librevenge-stream.h>

#include "ZMFTypes.h"

namespace test
{

using libzmf::BoundingBox;
using libzmf::Point;

class ZMFTypesTest : public CPPUNIT_NS::TestFixture
{
public:
  virtual void setUp() override;
  virtual void tearDown() override;

private:
  CPPUNIT_TEST_SUITE(ZMFTypesTest);
  CPPUNIT_TEST(testBBoxQuadrants);
  CPPUNIT_TEST_SUITE_END();

private:
  void testBBoxQuadrants();
};

void ZMFTypesTest::setUp()
{
}

void ZMFTypesTest::tearDown()
{
}

void ZMFTypesTest::testBBoxQuadrants()
{
  std::vector<BoundingBox> bboxes =
  {
    BoundingBox({Point(10, 10), Point(12, 10), Point(12, 12), Point(10, 12)}),
    // current implementation reverses bbox rotation before calculating quadrants,
    // so the first point will not be on the right side (quadrant 1 or 4) because in these cases it has rotation pi
    //BoundingBox({Point(12, 10), Point(10, 10), Point(10, 12), Point(12, 12)}),
    BoundingBox({Point(10, 12), Point(12, 12), Point(12, 10), Point(10, 10)}),
    //BoundingBox({Point(12, 12), Point(10, 12), Point(10, 10), Point(12, 10)}),

    BoundingBox({Point(10, 10), Point(14, 10), Point(14, 12), Point(10, 12)}),
    BoundingBox({Point(10, 10), Point(12, 10), Point(12, 14), Point(10, 14)}),
  };
  std::vector<int> p1Quadrants =
  {
    2,
    //1,
    3,
    //4,

    2,
    2,
  };
  std::vector<int> p2Quadrants =
  {
    1,
    //2,
    4,
    //3,

    1,
    1,
  };

  for (size_t i = 0; i < bboxes.size(); ++i)
  {
    CPPUNIT_ASSERT_EQUAL(p1Quadrants[i], bboxes[i].p1Quadrant());
    CPPUNIT_ASSERT_EQUAL(p2Quadrants[i], bboxes[i].p2Quadrant());
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(ZMFTypesTest);

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
