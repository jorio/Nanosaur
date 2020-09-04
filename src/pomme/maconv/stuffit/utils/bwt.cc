/*

Burrows-Wheeler Transform.

The code in this file is based on TheUnarchiver.
See README.md and docs/licenses/TheUnarchiver.txt for more information.

Copyright (C) 2019, Guillaume Gonnet

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "pomme/maconv/stuffit/utils/bwt.h"

namespace maconv {
    namespace stuffit {



        // Calculate inverse of BWT.
        void CalculateInverseBWT(uint32_t* transform, uint8_t* block, int block_len)
        {
            int counts[256] = { 0 };
            int cumulative_counts[256];

            for (int i = 0; i < block_len; i++)
                counts[block[i]]++;

            for (int i = 0, total = 0; i < 256; i++) {
                cumulative_counts[i] = total;
                total += counts[i];
                counts[i] = 0;
            }

            for (int i = 0; i < block_len; i++) {
                transform[cumulative_counts[block[i]] + counts[block[i]]] = i;
                counts[block[i]]++;
            }
        }



        // Reset the decoder.
        void MtfDecoder::ResetDecoder()
        {
            for (int i = 0; i < 256; i++)
                table[i] = i;
        }


        // Decode the next symbol.
        int MtfDecoder::Decode(int symbol)
        {
            int res = table[symbol];
            for (int i = symbol; i > 0; i--)
                table[i] = table[i - 1];

            table[0] = res;
            return res;
        }



    } // namespace stuffit
} // namespace maconv
