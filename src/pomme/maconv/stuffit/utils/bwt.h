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

#pragma once

#include <inttypes.h>

namespace maconv {
namespace stuffit {


// The MTF (Move-to-Front Transform) decoder.
struct MtfDecoder {

    // Reset the decoder.
    void ResetDecoder();

    // Decode the next symbol.
    int Decode(int symbol);

    int table[256];
};



// Calculate inverse of BWT.
void CalculateInverseBWT(uint32_t *transform, uint8_t *block, int block_len);


} // namespace stuffit
} // namespace maconv
