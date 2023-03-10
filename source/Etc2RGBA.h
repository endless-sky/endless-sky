/* ExclusiveItem.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef ETC2RGBA_H
#define ETC2RGBA_H

#include <cstdint>

class Etc2RGBA
{
public:
   Etc2RGBA(const void* data, int width, int height):
      m_block_width((width+3)/4), m_block_height((height+3)/4),
      m_data(reinterpret_cast<const uint64_t*>(data)) {}

   // TODO: do we need the ability to read the color? the alpha is encoded separately

   uint8_t Alpha(int frame, int x, int y)
   {
      // NOTE: this is the same as the R11 format used for red-only textures

      // pixels are encoded in 4x4 blocks, 8 bytes for alpha, followed by 8
      // bytes for rgba. Retrieve the alpha block.
      uint64_t alpha = *(m_data + (m_block_width * m_block_height * 2 * frame) +
                                  (m_block_width * 2 * (y/4)) + (x/4) * 2);

      // in big endian format, need to byteswap
      // TODO: is this defined anywhere for us? as-is, this gets converted by
      // gcc into a bswap on x86, and a rev on arm64
      alpha = (alpha & 0xff00000000000000) >> 56
            | (alpha & 0x00ff000000000000) >> 40
            | (alpha & 0x0000ff0000000000) >> 24
            | (alpha & 0x000000ff00000000) >>  8
            | (alpha & 0x00000000ff000000) <<  8
            | (alpha & 0x0000000000ff0000) << 24
            | (alpha & 0x000000000000ff00) << 40
            | (alpha & 0x00000000000000ff) << 56;


      // alpha represents a 4x4 block of pixels. Get the one we actually want
      int idx = ((x & 3) << 2) | (y & 3);
      uint8_t base_codeword = alpha >> 56;
      uint8_t multiplier = (alpha >> 52) & 0xf;
      uint8_t table_idx = (alpha >> 48) & 0xf;

      // pixels are 3 bits each, encoded from most significant to least
      // significant.
      int value_index = (alpha >> ((15 - idx) * 3)) & 7;

      // now we can compute the actual pixel
      static const int modifier_table[][8] = {
         { -3,  -6,  -9, -15,  2,  5,  8, 14 },
         { -3,  -7, -10, -13,  2,  6,  9, 12 },
         { -2,  -5,  -8, -13,  1,  4,  7, 12 },
         { -2,  -4,  -6, -13,  1,  3,  5, 12 },
         { -3,  -6,  -8, -12,  2,  5,  7, 11 },
         { -3,  -7,  -9, -11,  2,  6,  8, 10 },
         { -4,  -7,  -8, -11,  3,  6,  7, 10 },
         { -3,  -5,  -8, -11,  2,  4,  7, 10 },
         { -2,  -6,  -8, -10,  1,  5,  7,  9 },
         { -2,  -5,  -8, -10,  1,  4,  7,  9 },
         { -2,  -4,  -8, -10,  1,  3,  7,  9 },
         { -2,  -5,  -7, -10,  1,  4,  6,  9 },
         { -3,  -4,  -7, -10,  2,  3,  6,  9 },
         { -1,  -2,  -3, -10,  0,  1,  2,  9 },
         { -4,  -6,  -8,  -9,  3,  5,  7,  8 },
         { -3,  -5,  -7,  -9,  2,  4,  6,  8 },
      };
      int value = base_codeword + modifier_table[table_idx][value_index] * multiplier;
      if (value < 0) value = 0;
      else if (value > 255) value = 255;
      return value;
   }

private:
   const int m_block_width;
   const int m_block_height;
   const uint64_t* const m_data;
};

#endif