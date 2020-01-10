/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "libzmf_utils.h"

#include <unicode/ucnv.h>
#include <unicode/utypes.h>

#ifdef DEBUG
#include <cstdarg>
#include <cstdio>
#endif

#include <cstring>
#include <memory>

#include <boost/math/constants/constants.hpp>

namespace libzmf
{

namespace
{

void checkStream(const RVNGInputStreamPtr &input)
{
  if (!input || input->isEnd())
    throw EndOfStreamException();
}

struct SeekFailedException {};

static void _appendUCS4(librevenge::RVNGString &text, unsigned ucs4Character)
{
  unsigned char first;
  int len;
  if (ucs4Character < 0x80)
  {
    first = 0;
    len = 1;
  }
  else if (ucs4Character < 0x800)
  {
    first = 0xc0;
    len = 2;
  }
  else if (ucs4Character < 0x10000)
  {
    first = 0xe0;
    len = 3;
  }
  else if (ucs4Character < 0x200000)
  {
    first = 0xf0;
    len = 4;
  }
  else if (ucs4Character < 0x4000000)
  {
    first = 0xf8;
    len = 5;
  }
  else
  {
    first = 0xfc;
    len = 6;
  }

  char outbuf[7] = { 0 };
  int i;
  for (i = len - 1; i > 0; --i)
  {
    outbuf[i] = char((ucs4Character & 0x3f) | 0x80);
    ucs4Character >>= 6;
  }
  outbuf[0] = char((ucs4Character & 0xff) | first);
  outbuf[len] = '\0';

  text.append(outbuf);
}

}

uint8_t readU8(const RVNGInputStreamPtr &input, bool /* bigEndian */)
{
  checkStream(input);

  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint8_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint8_t))
    return *(uint8_t const *)(p);
  throw EndOfStreamException();
}

uint16_t readU16(const RVNGInputStreamPtr &input, bool bigEndian)
{
  checkStream(input);

  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint16_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint16_t))
  {
    if (bigEndian)
      return static_cast<uint16_t>((uint16_t)p[1]|((uint16_t)p[0]<<8));
    return static_cast<uint16_t>((uint16_t)p[0]|((uint16_t)p[1]<<8));
  }
  throw EndOfStreamException();
}

uint32_t readU32(const RVNGInputStreamPtr &input, bool bigEndian)
{
  checkStream(input);

  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint32_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint32_t))
  {
    if (bigEndian)
      return (uint32_t)p[3]|((uint32_t)p[2]<<8)|((uint32_t)p[1]<<16)|((uint32_t)p[0]<<24);
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
  }
  throw EndOfStreamException();
}

uint64_t readU64(const RVNGInputStreamPtr &input, bool bigEndian)
{
  checkStream(input);

  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint64_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint64_t))
  {
    if (bigEndian)
      return (uint64_t)p[7]|((uint64_t)p[6]<<8)|((uint64_t)p[5]<<16)|((uint64_t)p[4]<<24)|((uint64_t)p[3]<<32)|((uint64_t)p[2]<<40)|((uint64_t)p[1]<<48)|((uint64_t)p[0]<<56);
    return (uint64_t)p[0]|((uint64_t)p[1]<<8)|((uint64_t)p[2]<<16)|((uint64_t)p[3]<<24)|((uint64_t)p[4]<<32)|((uint64_t)p[5]<<40)|((uint64_t)p[6]<<48)|((uint64_t)p[7]<<56);
  }
  throw EndOfStreamException();
}

int32_t readS32(const RVNGInputStreamPtr &input, bool bigEndian)
{
  return (int32_t) readU32(input, bigEndian);
}

float readFloat(const RVNGInputStreamPtr &input, bool bigEndian)
{
  uint32_t u = readU32(input, bigEndian);
  float f;
  std::memcpy(&f, &u, sizeof(f));
  return f;
}

const unsigned char *readNBytes(const RVNGInputStreamPtr &input, const unsigned long numBytes)
{
  checkStream(input);

  unsigned long readBytes = 0;
  const unsigned char *const s = input->read(numBytes, readBytes);

  if (numBytes != readBytes)
    throw EndOfStreamException();

  return s;
}

void skip(const RVNGInputStreamPtr &input, unsigned long numBytes)
{
  checkStream(input);

  seekRelative(input, static_cast<long>(numBytes));
}

void seek(const RVNGInputStreamPtr &input, const unsigned long pos)
{
  if (!input)
    throw EndOfStreamException();
  if (0 != input->seek(static_cast<long>(pos), librevenge::RVNG_SEEK_SET))
    throw SeekFailedException();
}

void seekRelative(const RVNGInputStreamPtr &input, const long pos)
{
  if (!input)
    throw EndOfStreamException();
  if (0 != input->seek(pos, librevenge::RVNG_SEEK_CUR))
    throw SeekFailedException();
}

unsigned long getLength(const RVNGInputStreamPtr &input)
{
  checkStream(input);

  const long begin = input->tell();

  if (input->seek(0, librevenge::RVNG_SEEK_END) != 0)
  {
    // RVNG_SEEK_END does not work. Use the harder way.
    while (!input->isEnd())
      readU8(input);
  }
  long end = input->tell();
  if (end < begin)
    throw SeekFailedException();

  seek(input, begin);

  return static_cast<unsigned long>(end - begin);
}

void appendCharacters(librevenge::RVNGString &text, const unsigned char *characters, uint32_t size,
                      const char *encoding)
{
  if (size == 0)
  {
    ZMF_DEBUG_MSG(("Attempt to append 0 characters!"));
    return;
  }

  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<UConverter, void(*)(UConverter *)> conv(ucnv_open(encoding, &status), ucnv_close);
  if (U_SUCCESS(status))
  {
    // ICU documentation claims that character-by-character processing is faster "for small amounts of data" and "'normal' charsets"
    // (in any case, it is more convenient :) )
    const char *src = (const char *)&characters[0];
    const char *srcLimit = (const char *)src + size;
    while (src < srcLimit)
    {
      uint32_t ucs4Character = (uint32_t)ucnv_getNextUChar(conv.get(), &src, srcLimit, &status);
      if (U_SUCCESS(status))
      {
        _appendUCS4(text, ucs4Character);
      }
    }
  }
}

void writeU16(librevenge::RVNGBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
}

void writeU32(librevenge::RVNGBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
  buffer.append((unsigned char)((value >> 16) & 0xFF));
  buffer.append((unsigned char)((value >> 24) & 0xFF));
}

double rad2deg(double value)
{
  using namespace boost::math::double_constants;

  return normalizeAngle(value) * radian;
}

double normalizeAngle(double radAngle)
{
  using namespace boost::math::double_constants;
  radAngle = std::fmod(radAngle, two_pi);
  if (radAngle < 0)
    radAngle += two_pi;
  return radAngle;
}

#ifdef DEBUG
void debugPrint(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
}
#endif

EndOfStreamException::EndOfStreamException()
{
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
