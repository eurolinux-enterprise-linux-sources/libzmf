/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BMIParser.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include <png.h>

#include <zlib.h>

#include "ZMFCollector.h"

using librevenge::RVNGBinaryData;

namespace libzmf
{

namespace
{

double px2in(unsigned px, double dpi = 72)
{
  return px / dpi;
}

bool uncompress(const uint8_t *compressedData, unsigned size, std::vector<uint8_t> &uncompressedData)
{
  int ret;
  z_stream strm;

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit2(&strm, MAX_WBITS);
  if (ret != Z_OK)
    return false;

  const unsigned long blockSize = (std::max)(4096u, 2 * size);

  std::vector<uint8_t> data(blockSize);

  strm.avail_in = size;
  strm.next_in = (Bytef *)compressedData;
  strm.next_out = &data[0];

  bool done = false;
  while (!done)
  {
    const std::ptrdiff_t nextOutIndex = strm.next_out - &data[0];
    assert(nextOutIndex >= 0);
    data.resize(data.size() + blockSize);
    assert(data.size() > std::size_t(nextOutIndex));
    strm.avail_out = unsigned(data.size() - std::size_t(nextOutIndex));
    strm.next_out = reinterpret_cast<Bytef *>(&data[nextOutIndex]);

    ret = inflate(&strm, Z_SYNC_FLUSH);

    switch (ret)
    {
    case Z_OK:
      break;
    default:
      // abandon partially uncompressed data on error
      strm.total_out = 0;
      ZMF_FALLTHROUGH;
    case Z_STREAM_END:
      done = true;
      break;
    }
  }

  (void)inflateEnd(&strm);

  if (strm.total_out == 0)
    return false;

  std::move(data.begin(), std::next(data.begin(), strm.total_out), std::back_inserter(uncompressedData));

  return true;
}

}

namespace
{

extern "C" void pngWriteCallback(png_structp png, png_bytep data, png_size_t length) try
{
  auto p = reinterpret_cast<RVNGBinaryData *>(png_get_io_ptr(png));

  p->append(data, length);
}
catch (...)
{
  png_error(png, "pngWriteCallback exception");
}

extern "C" void pngFlushCallback(png_structp)
{ }

extern "C" void pngErrorCallback(png_structp png, png_const_charp msg);

class PNGWriter
{
public:
  struct Error
  {
    explicit Error(const std::string &msg)
      : m_msg(msg)
    {
    }

    const std::string &message() const
    {
      return m_msg;
    }

  private:
    const std::string m_msg;
  };

public:
  PNGWriter(RVNGBinaryData &output, const ColorBitmap &bitmap, const ColorBitmap &transparencyBitmap);

  void writeInfo();
  void writeData();
  void writeEnd();

  void setError(const std::string &error);

private:
  struct PNGInfoDeleter
  {
    PNGInfoDeleter(const std::shared_ptr<png_struct> &png)
      : m_png(png)
    {
    }

    void operator()(png_infop info) const
    {
      png_destroy_info_struct(m_png.get(), &info);
    }

  private:
    std::shared_ptr<png_struct> m_png;
  };

  typedef std::unique_ptr<png_info, PNGInfoDeleter> PNGInfoPtr;

private:
  std::shared_ptr<png_struct> createStruct();
  PNGInfoPtr createInfo(const std::shared_ptr<png_struct> &png) const;

  void writeRow();

  bool hasTransparency() const;

private:
  const ColorBitmap &m_bitmap;
  const ColorBitmap &m_transparencyBitmap;
  std::unique_ptr<png_byte[]> m_row;
  std::shared_ptr<png_struct> m_png;
  PNGInfoPtr m_info;
  std::string m_error;
};

PNGWriter::PNGWriter(RVNGBinaryData &output, const ColorBitmap &bitmap, const ColorBitmap &transparencyBitmap)
  : m_bitmap(bitmap)
  , m_transparencyBitmap(transparencyBitmap)
  , m_row()
  , m_png(createStruct())
  , m_info(createInfo(m_png))
  , m_error()
{
  png_set_write_fn(m_png.get(), &output, pngWriteCallback, pngFlushCallback);
  png_set_IHDR(m_png.get(), m_info.get(), bitmap.width, bitmap.height,
               8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  m_row.reset(new png_byte[4 * m_bitmap.width]);
}

std::shared_ptr<png_struct> PNGWriter::createStruct()
{
  const std::shared_ptr<png_struct> png(
    png_create_write_struct(PNG_LIBPNG_VER_STRING, this, pngErrorCallback, nullptr),
    [](png_structp p)
  {
    png_destroy_write_struct(&p, nullptr);
  });
  if (!png)
    throw Error("Could not allocate png_struct");
  return png;
}

PNGWriter::PNGInfoPtr PNGWriter::createInfo(const std::shared_ptr<png_struct> &png) const
{
  PNGInfoPtr info(png_create_info_struct(png.get()), PNGInfoDeleter(png));
  if (!info)
    throw Error("Could not allocate png_info");
  return info;
}

void PNGWriter::writeInfo()
{
  if (setjmp(png_jmpbuf(m_png.get())))
    throw Error(m_error);
  png_write_info(m_png.get(), m_info.get());
}

void PNGWriter::writeData()
{
  const png_bytep row = m_row.get();

  for (unsigned r = 0; r < m_bitmap.height; r++)
  {
    size_t rowByteInd = 0;

    for (unsigned c = 0; c < m_bitmap.width; c++)
    {
      size_t ind = r * m_bitmap.width + c;
      const Color &color = m_bitmap.data[ind];

      row[rowByteInd++] = color.red;
      row[rowByteInd++] = color.green;
      row[rowByteInd++] = color.blue;
      row[rowByteInd++] = (hasTransparency() && m_transparencyBitmap.data[ind].red != 0) ? 0 : 255;
    }

    writeRow();
  }
}

void PNGWriter::writeEnd()
{
  if (setjmp(png_jmpbuf(m_png.get())))
    throw Error(m_error);
  png_write_end(m_png.get(), nullptr);
}

void PNGWriter::setError(const std::string &error)
{
  m_error = error;
}

void PNGWriter::writeRow()
{
  if (setjmp(png_jmpbuf(m_png.get())))
    throw Error(m_error);
  png_write_row(m_png.get(), m_row.get());
}

bool PNGWriter::hasTransparency() const
{
  return !m_transparencyBitmap.data.empty();
}

extern "C" void pngErrorCallback(png_structp png, png_const_charp msg)
{
  try
  {
    PNGWriter *const writer = reinterpret_cast<PNGWriter *>(png_get_error_ptr(png));
    assert(writer);
    writer->setError(msg);
  }
  catch (...)
  {
    // Ignore. What else to do anyway?
  }
  longjmp(png_jmpbuf(png), -1);
}

bool makePng(RVNGBinaryData &output, const ColorBitmap &bitmap, const ColorBitmap &transparencyBitmap) try
{
  PNGWriter writer(output, bitmap, transparencyBitmap);
  writer.writeInfo();
  writer.writeData();
  writer.writeEnd();
  return true;
}
catch (const PNGWriter::Error &error)
{
  ZMF_DEBUG_MSG(("%s\n", error.message().c_str()));
  return false;
}

}

struct BMIParser::ColorBitmapHeader
{
  ColorBitmapHeader();

  void parse(const RVNGInputStreamPtr &input, const BMIHeader &header, const BMIOffset &offset);

  unsigned m_width;
  unsigned m_height;
  unsigned m_colorDepth;
  unsigned m_startOffset;
  unsigned m_endOffset;
};

BMIParser::ColorBitmapHeader::ColorBitmapHeader()
  : m_width(0)
  , m_height(0)
  , m_colorDepth(0)
  , m_startOffset(0)
  , m_endOffset(0)
{
}

void BMIParser::ColorBitmapHeader::parse(const RVNGInputStreamPtr &input, const BMIHeader &header, const BMIOffset &offset)
{
  seek(input, header.startOffset() + offset.start);
  m_endOffset = header.startOffset() + offset.end;

  m_width = readU16(input);
  m_height = readU16(input);

  const unsigned colorDepth = readU16(input);
  if (colorDepth <= 1)
    m_colorDepth = 1;
  else if (colorDepth <= 4)
    m_colorDepth = 4;
  else if (colorDepth <= 8)
    m_colorDepth = 8;
  else
    m_colorDepth = 24;

  m_startOffset = unsigned(static_cast<unsigned long>(input->tell()) + 10);
}

BMIParser::BMIParser(const RVNGInputStreamPtr &input, librevenge::RVNGDrawingInterface *const painter)
  : m_input(input)
  , m_painter(painter)
  , m_header()
{
}

bool BMIParser::parse()
{
  auto bitmap = readImage();

  if (bitmap.data.empty())
    return false;

  ZMFCollector collector(m_painter);

  collector.startDocument();
  collector.startPage(ZMFPageSettings(8.5, 11, 0, 0));
  collector.startLayer();

  collector.collectImage(bitmap.data, Point(0, 0), px2in(m_header.width()), px2in(m_header.height()), 0, false, false);

  collector.endLayer();
  collector.endPage();
  collector.endDocument();

  return true;
}

Image BMIParser::readImage()
{
  if (!m_header.load(m_input) || !m_header.isSupported())
    return Image();

  boost::optional<ColorBitmapHeader> bitmapHeader;
  boost::optional<ColorBitmapHeader> transparencyBitmapHeader;

  for (const auto &offset : m_header.offsets())
  {
    if (offset.type == BMIStreamType::BITMAP)
    {
      if (!bitmapHeader)
      {
        bitmapHeader = ColorBitmapHeader();
        bitmapHeader->parse(m_input, m_header, offset);
      }
      else if (!transparencyBitmapHeader)
      {
        transparencyBitmapHeader = ColorBitmapHeader();
        transparencyBitmapHeader->parse(m_input, m_header, offset);
      }
    }
  }

  if (!bitmapHeader)
    return Image();

  if (transparencyBitmapHeader)
  {
    if (!reconcileDimensions(get(bitmapHeader), get(transparencyBitmapHeader)))
      return Image();
  }

  ColorBitmap bitmap = readColorBitmap(get(bitmapHeader));
  ColorBitmap transparencyBitmap;
  if (transparencyBitmapHeader)
    transparencyBitmap = readColorBitmap(get(transparencyBitmapHeader));

  if (bitmap.width == 0 || bitmap.height == 0)
    return Image();

  Image result;
  result.width = bitmap.width;
  result.height = bitmap.height;
  if (makePng(result.data, bitmap, transparencyBitmap))
  {
    return result;
  }

  return Image();
}

const BMIHeader &BMIParser::header() const
{
  return m_header;
}

ColorBitmap BMIParser::readColorBitmap(const BMIParser::ColorBitmapHeader &hdr)
{
  ColorBitmap bitmap;

  bitmap.width = hdr.m_width;
  bitmap.height = hdr.m_height;

  seek(m_input, hdr.m_startOffset);

  std::vector<Color> colorPalette;
  if (hdr.m_colorDepth < 24)
  {
    colorPalette = readColorPalette(hdr.m_colorDepth);
  }

  auto data = readData(hdr.m_endOffset);

  unsigned lineBitCount = bitmap.width * hdr.m_colorDepth;

  unsigned lineWidth = lineBitCount / 8;
  if (lineBitCount % 8 != 0)
    lineWidth++;

  unsigned padding = 0;
  while ((lineWidth + padding) % 4 != 0)
    padding++;

  lineWidth += padding;

  unsigned shift = 8 - std::min<uint16_t>(hdr.m_colorDepth, 8);
  unsigned mask = (0xff >> shift) << shift;

  if (data.size() < bitmap.height * lineWidth)
    return ColorBitmap();

  bitmap.data.reserve(bitmap.width * bitmap.height);

  unsigned i = 0;

  for (unsigned row = 0; row  < bitmap.height; row++)
  {
    unsigned col = 0;

    while (col < bitmap.width)
    {
      if (hdr.m_colorDepth == 24)
      {
        Color color;
        color.blue = data[i++];
        color.green = data[i++];
        color.red = data[i++];

        bitmap.data.push_back(color);

        col++;
      }
      else
      {
        uint8_t indexes = data[i++];
        for (unsigned j = 0; j < 8 / hdr.m_colorDepth; j++)
        {
          uint8_t index = (indexes & mask) >> shift;

          bitmap.data.push_back(colorPalette[index]);

          indexes = indexes << hdr.m_colorDepth;

          col++;
          if (col == bitmap.width)
            break;
        }
      }
    }

    i += padding;
  }

  return bitmap;
}

std::vector<uint8_t> BMIParser::readData(unsigned endOffset)
{
  std::vector<uint8_t> data;

  while (m_input->tell() < endOffset)
  {
    auto blockSize = readU16(m_input);

    skip(m_input, 1);

    auto compressedBlock = readNBytes(m_input, blockSize);

    if (!uncompress(compressedBlock, blockSize, data))
    {
      data.clear();
      break;
    }
  }

  return data;
}

std::vector<Color> BMIParser::readColorPalette(unsigned colorDepth)
{
  unsigned colorCount = 1 << colorDepth;

  std::vector<Color> colors(colorCount);
  for (auto &color : colors)
  {
    color.blue = readU8(m_input);
    color.green = readU8(m_input);
    color.red = readU8(m_input);
    skip(m_input, 1);
  }

  return colors;
}

bool BMIParser::reconcileDimensions(BMIParser::ColorBitmapHeader &colorHeader, BMIParser::ColorBitmapHeader &transparencyHeader)
{
  return m_header.reconcileWidth(colorHeader.m_width, transparencyHeader.m_width)
         && m_header.reconcileHeight(colorHeader.m_height, transparencyHeader.m_height);
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
