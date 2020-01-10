/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is a part of the libzmf project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>

#include <libzmf/libzmf.h>

#ifndef PACKAGE
#define PACKAGE "libzmf"
#endif
#ifndef VERSION
#define VERSION "UNKNOWN VERSION"
#endif

namespace
{

int printUsage()
{
  printf("`zmf2raw' is used to test " PACKAGE ".\n");
  printf("\n");
  printf("Usage: zmf2raw [OPTION] INPUT\n");
  printf("\t--callgraph           display the call graph nesting level\n");
  printf("\t--help                show this help message\n");
  printf("\t--version             show version information and exit\n");
  return -1;
}

int printVersion()
{
  printf("zmf2raw " VERSION "\n");
  return 0;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
  if (argc < 2)
    return printUsage();

  char *file = 0;
  bool printCallgraph = false;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "--callgraph"))
      printCallgraph = true;
    else if (!strcmp(argv[i], "--version"))
      return printVersion();
    else if (!file && strncmp(argv[i], "--", 2))
      file = argv[i];
    else
      return printUsage();
  }

  if (!file)
    return printUsage();

  librevenge::RVNGFileStream input(file);

  if (!libzmf::ZMFDocument::isSupported(&input))
  {
    fprintf(stderr, "ERROR: Unsupported file format (unsupported version) or file is encrypted!\n");
    return 1;
  }

  librevenge::RVNGRawDrawingGenerator painter(printCallgraph);
  libzmf::ZMFDocument::parse(&input, &painter);

  return 0;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
