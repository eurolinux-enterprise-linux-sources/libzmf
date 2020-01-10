/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LIBZMF_UTILS_H_INCLUDED
#define LIBZMF_UTILS_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cmath>
#include <memory>
#include <string>
#include <bitset>

#include <boost/cstdint.hpp>

#include <librevenge-stream/librevenge-stream.h>
#include <librevenge/librevenge.h>

#ifdef DEBUG
#include <boost/type_index.hpp>
#include <regex>
#endif

#define ZMF_EPSILON 1E-6
#define ZMF_ALMOST_ZERO(m) (std::fabs(m) <= ZMF_EPSILON)

#if defined(HAVE_CLANG_ATTRIBUTE_FALLTHROUGH)
#  define ZMF_FALLTHROUGH [[clang::fallthrough]]
#elif defined(HAVE_GCC_ATTRIBUTE_FALLTHROUGH)
#  define ZMF_FALLTHROUGH __attribute__((fallthrough))
#else
#  define ZMF_FALLTHROUGH ((void) 0)
#endif

#ifdef DEBUG

#if defined(HAVE_FUNC_ATTRIBUTE_FORMAT)
#define ZMF_ATTRIBUTE_PRINTF(fmt, arg) __attribute__((__format__(__printf__, fmt, arg)))
#else
#define ZMF_ATTRIBUTE_PRINTF(fmt, arg)
#endif

#define ZMF_DEBUG_MSG(M) libzmf::debugPrint M
#define ZMF_DEBUG(M) M

#else // !DEBUG

// do nothing with debug messages in a release compile
#define ZMF_DEBUG_MSG(M)
#define ZMF_DEBUG(M)

#endif // DEBUG

#define ZMF_NUM_ELEMENTS(array) sizeof(array)/sizeof(array[0])

namespace libzmf
{

template<typename T>
std::string prettyTypeName()
{
#ifdef DEBUG
  auto str = boost::typeindex::type_id<T>().pretty_name();
  str = std::regex_replace(str, std::regex("libzmf::"), "");
  str = std::regex_replace(str, std::regex("boost::"), "");
  return str;
#else
  return "";
#endif
}

typedef std::shared_ptr<librevenge::RVNGInputStream> RVNGInputStreamPtr;

struct ZMFDummyDeleter
{
  void operator()(void *) {}
};

uint8_t readU8(const RVNGInputStreamPtr &input, bool = false);
uint16_t readU16(const RVNGInputStreamPtr &input, bool bigEndian=false);
uint32_t readU32(const RVNGInputStreamPtr &input, bool bigEndian=false);
uint64_t readU64(const RVNGInputStreamPtr &input, bool bigEndian=false);
int32_t readS32(const RVNGInputStreamPtr &input, bool bigEndian=false);

float readFloat(const RVNGInputStreamPtr &input, bool bigEndian=false);

const unsigned char *readNBytes(const RVNGInputStreamPtr &input, unsigned long numBytes);

void skip(const RVNGInputStreamPtr &input, unsigned long numBytes);

void seek(const RVNGInputStreamPtr &input, unsigned long pos);
void seekRelative(const RVNGInputStreamPtr &input, long pos);

unsigned long getLength(const RVNGInputStreamPtr &input);

void appendCharacters(librevenge::RVNGString &text, const unsigned char *characters, uint32_t size,
                      const char *encoding);

void writeU16(librevenge::RVNGBinaryData &buffer, const int value);
void writeU32(librevenge::RVNGBinaryData &buffer, const int value);

double rad2deg(double value);
double normalizeAngle(double radAngle);

template<std::size_t numBytes>
std::bitset<numBytes * 8> bytesToBitset(const uint8_t *data)
{
  std::bitset<numBytes * 8> b;

  for (std::size_t i = 0; i < numBytes; ++i)
  {
    uint8_t cur = data[i];
    std::size_t offset = i * 8;

    for (int j = 0; j < 8; ++j)
    {
      b[offset++] = cur & 1;
      cur >>= 1;
    }
  }

  return b;
}

template<typename T>
double um2in(T micrometers)
{
  return micrometers / 1000.0 / 25.4;
}

#ifdef DEBUG
void debugPrint(const char *format, ...) ZMF_ATTRIBUTE_PRINTF(1, 2);
#endif

struct EndOfStreamException
{
  EndOfStreamException();
};

struct GenericException
{
};

}

#endif // LIBZMF_UTILS_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
