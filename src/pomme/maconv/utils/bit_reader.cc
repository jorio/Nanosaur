/*

Read a buffer by group of bits (not necessarily multiple of 8).

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

#include "pomme/maconv/utils/bit_reader.h"

#include <algorithm>

namespace maconv {
    namespace utils {



        // Load a buffer of |length| in the reader.
        void BitReader::Load(uint8_t* data, int length)
        {
            this->data = data;
            this->end = data + length;
        }


        // Ignore a number of bits (n can be > 32).
        void BitReader::IgnoreBits(int n)
        {
            for (; n > 0; n -= 24)
                ReadWord(std::min(n, 24));
        }



        // Refill the bit cache.
        void BitReaderBE::FillBitCache()
        {
            int num_bytes = std::min((32 - num_bits) / 8, (int)(end - data));
            num_bits += (8 * num_bytes);

            for (int i = 0; i < num_bytes; i++)
                bits = (bits << 8) | *(data++);
        }


        // Read a word (i.e. n <= 32 bits).
        uint32_t BitReaderBE::ReadWord(int n, bool skip)
        {
            if (n > num_bits)
                FillBitCache();

            uint32_t ret = (bits >> (num_bits - n)) & ((1 << n) - 1);
            if (skip) SkipBits(n);
            return ret;
        }


        // Read a long word (n can be > 25 bits).
        uint32_t BitReaderBE::ReadLongWord(int n, bool skip)
        {
            if (n <= 25) return ReadWord(n, skip);
            int bits = ReadWord(25, skip) << (n - 25);
            return bits | ReadWord(n - 25, skip);
        }


        // Skip some bits (that has been readed).
        void BitReaderBE::SkipBits(int n)
        {
            num_bits -= n;
        }



        // Refill the bit cache.
        void BitReaderLE::FillBitCache()
        {
            int num_bytes = std::min((32 - num_bits) / 8, (int)(end - data));

            for (int i = 0; i < num_bytes; i++, num_bits += 8)
                bits |= *(data++) << num_bits;
        }


        // Read a word (i.e. n <= 32 bits).
        uint32_t BitReaderLE::ReadWord(int n, bool skip)
        {
            if (n > num_bits)
                FillBitCache();

            uint32_t ret = bits & ((1 << n) - 1);
            if (skip) SkipBits(n);
            return ret;
        }


        // Read a long word (n can be > 25 bits).
        uint32_t BitReaderLE::ReadLongWord(int n, bool skip)
        {
            if (n <= 25) return ReadWord(n, skip);
            int bits = ReadWord(25, skip);
            return (ReadWord(n - 25, skip) << 25) | bits;
        }


        // Skip some bits (that has been readed).
        void BitReaderLE::SkipBits(int n)
        {
            bits >>= n;
            num_bits -= n;
        }



    } // namespace utils
} // namespace maconv
