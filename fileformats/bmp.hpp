/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

/// \nodoc (keyword to exclude this file from automatic README.md generation)

#pragma once

#include <stdio.h>

inline void saveBMP(const char* bmpfilename, int width, int height, const unsigned char* bgra)
{
#pragma pack(push, 1)
  struct
  {
    unsigned short bfType;
    unsigned int   bfSize;
    unsigned int   bfReserved;
    unsigned int   bfOffBits;

    unsigned int   biSize;
    signed int     biWidth;
    signed int     biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    signed int     biXPelsPerMeter;
    signed int     biYPelsPerMeter;
    unsigned int   biClrUsed;
    unsigned int   biClrImportant;
  } bmpinfo;
#pragma pack(pop)

  const unsigned int imageDataSize = width * height * 4 * static_cast<unsigned int>(sizeof(unsigned char));

  bmpinfo.bfType     = 19778;
  bmpinfo.bfSize     = static_cast<unsigned int>(sizeof(bmpinfo)) + imageDataSize;
  bmpinfo.bfReserved = 0;
  bmpinfo.bfOffBits  = 54;

  bmpinfo.biSize          = 40;
  bmpinfo.biWidth         = width;
  bmpinfo.biHeight        = height;
  bmpinfo.biPlanes        = 1;
  bmpinfo.biBitCount      = 32;
  bmpinfo.biCompression   = 0;
  bmpinfo.biSizeImage     = 0;
  bmpinfo.biXPelsPerMeter = 0;
  bmpinfo.biYPelsPerMeter = 0;
  bmpinfo.biClrUsed       = 0;
  bmpinfo.biClrImportant  = 0;

  FILE* bmpfile = fopen(bmpfilename, "wb");
  fwrite(&bmpinfo, sizeof(bmpinfo), 1, bmpfile);
  fwrite(bgra, sizeof(char), imageDataSize, bmpfile);
  fclose(bmpfile);
}