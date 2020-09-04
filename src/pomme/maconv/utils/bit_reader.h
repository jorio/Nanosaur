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

#pragma once

#include <inttypes.h>

namespace maconv {
    namespace utils {


        // Read a buffer by group of bits (not necessarily multiple of 8).
        struct BitReader {

            // Load a buffer of |length| in the reader.
            void Load(uint8_t* data, int length);


            // Is the reader at end?
            bool HasEnded(int n = 0) { return data == end && num_bits <= n; }

            // Ignore a number of bits (n can be > 32).
            void IgnoreBits(int n);


            // Read a single bit.
            virtual uint8_t ReadBit() { return ReadWord(1); };

            // Read a word (i.e. n <= 25 bits).
            virtual uint32_t ReadWord(int n, bool skip = true) = 0;

            // Read a long word (n can be > 25).
            virtual uint32_t ReadLongWord(int n, bool skip = true) = 0;

            // Skip some bits (that has been readed).
            virtual void SkipBits(int n) = 0;


            uint8_t* data; // Current pointer on data.
            uint8_t* end; // Length of the buffer.

            uint32_t bits = 0; // Bit cache.
            int num_bits = 0; // Number of bits in the cache.
        };



        // BitReader that reads Big Endian integers.
        struct BitReaderBE : BitReader {

            // Read a word (i.e. <= 32 bits).
            uint32_t ReadWord(int n, bool skip = true) override;

            // Read a long word (n can be > 25 bits).
            uint32_t ReadLongWord(int n, bool skip = true) override;

            // Skip some bits (that has been readed).
            void SkipBits(int n) override;

            // Refill the bit cache.
            void FillBitCache();
        };


        // BitReader that reads Little Endian integers.
        struct BitReaderLE : BitReader {

            // Read a word (i.e. <= 25 bits).
            uint32_t ReadWord(int n, bool skip = true) override;

            // Read a long word (n can be > 25 bits).
            uint32_t ReadLongWord(int n, bool skip = true) override;

            // Skip some bits (that has been readed).
            void SkipBits(int n) override;

            // Refill the bit cache.
            void FillBitCache();
        };


    } // namespace utils
} // namespace maconv
