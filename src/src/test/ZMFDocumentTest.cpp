/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <librevenge-stream/librevenge-stream.h>

#include <libzmf/libzmf.h>

#if !defined DETECTION_TEST_DIR
#error DETECTION_TEST_DIR not defined, cannot test
#endif

namespace test
{

using libzmf::ZMFDocument;

using std::string;

namespace
{

void assertDetection(const string &name, const bool expectedSupported, const ZMFDocument::Type *const expectedType = 0, const ZMFDocument::Kind *const expectedKind = 0)
{
  librevenge::RVNGFileStream input((string(DETECTION_TEST_DIR) + "/" + name).c_str());
  ZMFDocument::Kind kind = ZMFDocument::KIND_UNKNOWN;
  ZMFDocument::Type type = ZMFDocument::TYPE_UNKNOWN;
  const bool supported = ZMFDocument::isSupported(&input, &type, &kind);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(name + ": supported", expectedSupported, supported);
  if (expectedType)
    CPPUNIT_ASSERT_EQUAL_MESSAGE(name + ": type", *expectedType, type);
  if (expectedKind)
    CPPUNIT_ASSERT_EQUAL_MESSAGE(name + ": kind", *expectedKind, kind);
}

void assertSupported(const string &name, const ZMFDocument::Type type, const ZMFDocument::Kind kind)
{
  assertDetection(name, true, &type, &kind);
}

void assertUnsupported(const string &name)
{
  assertDetection(name, false);
}

}

class ZMFDocumentTest : public CPPUNIT_NS::TestFixture
{
public:
  virtual void setUp() override;
  virtual void tearDown() override;

private:
  CPPUNIT_TEST_SUITE(ZMFDocumentTest);
  CPPUNIT_TEST(testDetectDraw);
  CPPUNIT_TEST(testDetectZebra);
  CPPUNIT_TEST(testDetectBitmap);
  CPPUNIT_TEST(testUnsupported);
  CPPUNIT_TEST_SUITE_END();

private:
  void testDetectDraw();
  void testDetectZebra();
  void testDetectBitmap();
  void testUnsupported();
};

void ZMFDocumentTest::setUp()
{
}

void ZMFDocumentTest::tearDown()
{
}

void ZMFDocumentTest::testDetectZebra()
{
  assertSupported("zebra.zbr", ZMFDocument::TYPE_ZEBRA, ZMFDocument::KIND_DRAW);
}

void ZMFDocumentTest::testDetectDraw()
{
  const ZMFDocument::Type type(ZMFDocument::TYPE_DRAW);
  const ZMFDocument::Kind kind(ZMFDocument::KIND_DRAW);

  // version 2-3
  // assertSupported("draw2.zmf", type, kind);
  // assertSupported("draw3.zmf", type, kind);

  // version 4-5
  assertSupported("draw4.zmf", type, kind);
  assertSupported("draw5.zmf", type, kind);
  assertSupported("draw5-uncompressed.zmf", type, kind);
}

void ZMFDocumentTest::testDetectBitmap()
{
  const ZMFDocument::Type type(ZMFDocument::TYPE_BITMAP);
  const ZMFDocument::Kind kind(ZMFDocument::KIND_PAINT);

  assertSupported("bitmap.bmi", type, kind);
}

void ZMFDocumentTest::testUnsupported()
{
  // ensure the detection is not too broad
  assertUnsupported("unsupported.zip");
}

CPPUNIT_TEST_SUITE_REGISTRATION(ZMFDocumentTest);

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
