/**************************************************************************
 * Copyright (C) 2021-2021  Junlon2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : yuv422_rotation.c
 * Author      : junlon2006@163.com
 * Date        : 2020.06.14
 *
 **************************************************************************/
#include <stdint.h>
#include <stdio.h>

uint8_t* yuv422_uyvy_rotate_clockwise_90(uint8_t *dst, uint8_t *src, uint32_t width, uint32_t height)
{
    const int copyBytes    = 4;
    const int bytesPerLine = width << 1;
    const int step         = height << 2;
    const int offset       = (height - 1) * bytesPerLine;
    uint8_t *dest;
    uint8_t *psrc;
    uint8_t *pdst[2];
    int i, j, k;
    uint8_t temp;

    dest = dst;
    for (i = 0; i < bytesPerLine; i += copyBytes) {
        pdst[0] = dest;
        pdst[1] = dest + (height << 1);
        psrc = src + offset;

        for (j = 0; j < height; ++j) {
            k = (j & 1);

            *((uint32_t *)pdst[k]) = *((uint32_t *)psrc);

            if (1 == k) {
                temp = *(pdst[0] - 1);
                *(pdst[0] - 1) = *(pdst[1] + 1);
                *(pdst[1] + 1) = temp;
            }

            pdst[k] += copyBytes;
            psrc    -= bytesPerLine;
        }

        dest += step;
        src  += copyBytes;
    }

    return dst;
}