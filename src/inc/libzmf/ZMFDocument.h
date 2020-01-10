/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LIBZMF_ZMFDOCUMENT_H_INCLUDED
#define LIBZMF_ZMFDOCUMENT_H_INCLUDED

#include <librevenge/librevenge.h>

#ifdef DLL_EXPORT
#ifdef LIBZMF_BUILD
#define ZMFAPI __declspec(dllexport)
#else
#define ZMFAPI __declspec(dllimport)
#endif
#else // !DLL_EXPORT
#ifdef LIBZMF_VISIBILITY
#define ZMFAPI __attribute__((visibility("default")))
#else
#define ZMFAPI
#endif
#endif

namespace libzmf
{

class ZMFDocument
{
public:
  enum Type
  {
    TYPE_UNKNOWN, ///< Unrecognized file
    TYPE_DRAW, ///< Zoner Draw/Callisto (v. 4-5)
    TYPE_ZEBRA, ///< Zoner Zebra
    TYPE_BITMAP ///< Zoner Bitmap
  };

  enum Kind
  {
    KIND_UNKNOWN,
    KIND_DRAW,
    KIND_PAINT
  };

  /**
    Analyzes the content of an input stream to see if it can be
    parsed.

    \param input The input stream
    \param[out] type Type of the document
    \return A value that
    indicates whether the content from the input stream is a
    document that libzmf is able to parse
  */
  static ZMFAPI bool isSupported(librevenge::RVNGInputStream *input, Type *type = 0, Kind *kind = 0);

  /**
    Parses the input stream content.

    It will make callbacks to the functions provided by a
    librevenge::RVNGDrawingInterface class implementation when needed.

    \param input The input stream
    \param painter A librevenge::RVNGDrawingInterface implementation
    \return A value that indicates whether the parsing was successful
  */
  static ZMFAPI bool parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter);
};

} // namespace libzmf

#endif // LIBZMF_ZMFDOCUMENT_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
