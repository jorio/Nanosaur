/*

Arsenic algorithm: BWT and arithmetic coding.

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

#include "maconv/stuffit/methods.h"
#include "maconv/stuffit/utils/bwt.h"
#include "maconv/stuffit/utils/crc.h"
#include "maconv/utils/bit_reader.h"

#include <memory>

namespace maconv {
namespace stuffit {


// Store an arithmetic symbol.
struct ArithmeticSymbol {
    int symbol;
    int freq;
};


// The arithmetic model.
struct ArithmeticModel {

    // Initialize the model with some values.
    void Initialize(int firstsymb, int lastsym, int incr, int freqlimit);

    // Reset the model.
    void ResetModel();

    // Increase the model frequency at |symindex| by |increment|.
    void IncreaseFrequency(int symindex);

    int total_freq;
    int increment;
    int freq_limit;

    int num_symbols;
    ArithmeticSymbol symbols[128];
};


// The arithmetic decoder.
struct ArithmeticDecoder {

    // Initialize the decoder with some values.
    void Initialize(uint8_t *data, uint32_t length);

    // Get the next arithmetic code.
    void NextCode(int symlow, int symsize, int symtot);

    // Get the next arithmetic symbol.
    int NextSymbol(ArithmeticModel *model);

    // Get the next word (that has |n| bits).
    int NextWord(ArithmeticModel *model, int n);

    utils::BitReaderBE input;
    int range, code;
};



// Arsenic compression algorithm.
struct ArsenicMethod : CompressionMethod {

    // Initialize the algorithm.
    void Initialize() override;

    // Read the next block.
    void ReadNextBlock();


    // Read the next byte.
    int32_t ReadNextByte();

    // Read the next bytes.
    int32_t ReadBytes(uint8_t *data, uint32_t length) override;


    ArithmeticModel initial_model, selector_model, mtf_model[7];
    ArithmeticDecoder decoder;
    MtfDecoder mtf;

    std::unique_ptr<uint8_t[]> block;
    int block_bits, block_size;
    bool end_of_blocks;

    int num_bytes, byte_count, transform_index;
    std::unique_ptr<uint32_t[]> transform;

    int randomized, rand_count, rand_index;
    int repeat, count, last;

    uint32_t crc, compcrc;
};


} // namespace stuffit
} // namespace maconv
