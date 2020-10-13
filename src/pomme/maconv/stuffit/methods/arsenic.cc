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

#include "maconv/stuffit/methods/arsenic.h"
//#include "commands.h"

#include <memory>
#include <string.h>

namespace maconv {
namespace stuffit {



// The randomization table.
static const uint16_t kRandomizationTable[] = {
    0xEE,  0x56,  0xF8,  0xC3,  0x9D,  0x9F,  0xAE,  0x2C,
    0xAD,  0xCD,  0x24,  0x9D,  0xA6, 0x101,  0x18,  0xB9,
    0xA1,  0x82,  0x75,  0xE9,  0x9F,  0x55,  0x66,  0x6A,
    0x86,  0x71,  0xDC,  0x84,  0x56,  0x96,  0x56,  0xA1,
    0x84,  0x78,  0xB7,  0x32,  0x6A,   0x3,  0xE3,   0x2,
    0x11, 0x101,   0x8,  0x44,  0x83, 0x100,  0x43,  0xE3,
    0x1C,  0xF0,  0x86,  0x6A,  0x6B,   0xF,   0x3,  0x2D,
    0x86,  0x17,  0x7B,  0x10,  0xF6,  0x80,  0x78,  0x7A,
    0xA1,  0xE1,  0xEF,  0x8C,  0xF6,  0x87,  0x4B,  0xA7,
    0xE2,  0x77,  0xFA,  0xB8,  0x81,  0xEE,  0x77,  0xC0,
    0x9D,  0x29,  0x20,  0x27,  0x71,  0x12,  0xE0,  0x6B,
    0xD1,  0x7C,   0xA,  0x89,  0x7D,  0x87,  0xC4, 0x101,
    0xC1,  0x31,  0xAF,  0x38,   0x3,  0x68,  0x1B,  0x76,
    0x79,  0x3F,  0xDB,  0xC7,  0x1B,  0x36,  0x7B,  0xE2,
    0x63,  0x81,  0xEE,   0xC,  0x63,  0x8B,  0x78,  0x38,
    0x97,  0x9B,  0xD7,  0x8F,  0xDD,  0xF2,  0xA3,  0x77,
    0x8C,  0xC3,  0x39,  0x20,  0xB3,  0x12,  0x11,   0xE,
    0x17,  0x42,  0x80,  0x2C,  0xC4,  0x92,  0x59,  0xC8,
    0xDB,  0x40,  0x76,  0x64,  0xB4,  0x55,  0x1A,  0x9E,
    0xFE,  0x5F,   0x6,  0x3C,  0x41,  0xEF,  0xD4,  0xAA,
    0x98,  0x29,  0xCD,  0x1F,   0x2,  0xA8,  0x87,  0xD2,
    0xA0,  0x93,  0x98,  0xEF,   0xC,  0x43,  0xED,  0x9D,
    0xC2,  0xEB,  0x81,  0xE9,  0x64,  0x23,  0x68,  0x1E,
    0x25,  0x57,  0xDE,  0x9A,  0xCF,  0x7F,  0xE5,  0xBA,
    0x41,  0xEA,  0xEA,  0x36,  0x1A,  0x28,  0x79,  0x20,
    0x5E,  0x18,  0x4E,  0x7C,  0x8E,  0x58,  0x7A,  0xEF,
    0x91,   0x2,  0x93,  0xBB,  0x56,  0xA1,  0x49,  0x1B,
    0x79,  0x92,  0xF3,  0x58,  0x4F,  0x52,  0x9C,   0x2,
    0x77,  0xAF,  0x2A,  0x8F,  0x49,  0xD0,  0x99,  0x4D,
    0x98, 0x101,  0x60,  0x93, 0x100,  0x75,  0x31,  0xCE,
    0x49,  0x20,  0x56,  0x57,  0xE2,  0xF5,  0x26,  0x2B,
    0x8A,  0xBF,  0xDE,  0xD0,  0x83,  0x34,  0xF4,  0x17
};




// Initialize the model with some values.
void ArithmeticModel::Initialize(int first_symbol, int last_symbol, int increment,
    int frequency_limit)
{
    this->increment = increment;
    this->freq_limit = frequency_limit;
    this->num_symbols = last_symbol - first_symbol + 1;

    ResetModel();
    for (int i = 0; i < num_symbols; i++)
        symbols[i].symbol = i + first_symbol;
}


// Reset the model.
void ArithmeticModel::ResetModel()
{
    total_freq = increment * num_symbols;
    for (int i = 0; i < num_symbols; i++)
        symbols[i].freq = increment;
}


// Increase the model frequency at |symindex| by |increment|.
void ArithmeticModel::IncreaseFrequency(int symindex)
{
    symbols[symindex].freq += increment;

    total_freq += increment;
    if (total_freq <= freq_limit)
        return;

    total_freq = 0;
    for (int i = 0; i < num_symbols; i++) {
        symbols[i].freq++;
        symbols[i].freq >>= 1;
        total_freq += symbols[i].freq;
    }
}



// Decoder constants.
constexpr int kDecoderNumBits = 26;
constexpr int kDecoderOne = (1 << (kDecoderNumBits - 1));
constexpr int kDecoderHalf = (1 << (kDecoderNumBits - 2));


// Initialize the decoder with some values.
void ArithmeticDecoder::Initialize(uint8_t *data, uint32_t length)
{
    input.Load(data, length);
    range = kDecoderOne;
    code = input.ReadLongWord(kDecoderNumBits);
}


// Get the next arithmetic code.
void ArithmeticDecoder::NextCode(int symlow, int symsize, int symtot)
{
    int renormf = range / symtot;
    int lowincr = renormf * symlow;

    code -= lowincr;
    range = (symlow + symsize == symtot) ? (range - lowincr) : (symsize * renormf);

    for (; range <= kDecoderHalf; range <<= 1)
        code = (code << 1) | input.ReadBit();
}


// Get the next arithmetic symbol.
int ArithmeticDecoder::NextSymbol(ArithmeticModel *model)
{
    int freq = code / (range / model->total_freq);
    int cumulative = 0, n = 0;

    for (; n < model->num_symbols - 1; n++) {
        if (cumulative + model->symbols[n].freq > freq) break;
        cumulative += model->symbols[n].freq;
    }

    NextCode(cumulative, model->symbols[n].freq, model->total_freq);
    model->IncreaseFrequency(n);

    return model->symbols[n].symbol;
}


// Get the next word (that has |n| bits).
int ArithmeticDecoder::NextWord(ArithmeticModel *model, int n)
{
    int word = 0;
    for (int i = 0; i < n; i++) {
        if (NextSymbol(model))
            word |= (1 << i);
    }

    return word;
}




// Initialize the algorithm.
void ArsenicMethod::Initialize()
{
    decoder.Initialize(data, end - data);

    initial_model.Initialize(0, 1, 1, 256);
    selector_model.Initialize(0, 10, 8, 1024);
    mtf_model[0].Initialize(2, 3, 8, 1024);
    mtf_model[1].Initialize(4, 7, 4, 1024);
    mtf_model[2].Initialize(8, 15, 4, 1024);
    mtf_model[3].Initialize(16, 31, 4, 1024);
    mtf_model[4].Initialize(32, 63, 2, 1024);
    mtf_model[5].Initialize(64, 127, 2, 1024);
    mtf_model[6].Initialize(128, 255, 1, 1024);

    if (decoder.NextWord(&initial_model, 8) != 'A')
        throw ExtractException("Arsenic: invalid compressed data [A]");
    if (decoder.NextWord(&initial_model, 8) != 's')
        throw ExtractException("Arsenic: invalid compressed data [s]");

    block_bits = decoder.NextWord(&initial_model, 4) + 9;
    block_size = (1 << block_bits);

    num_bytes = 0; byte_count = 0; repeat = 0;
    crc = 0xFFFFFFFF; compcrc = 0;

    block = std::make_unique<uint8_t[]>(block_size);
    end_of_blocks = decoder.NextSymbol(&initial_model); // Check first end marker.
}



// Read the next block.
void ArsenicMethod::ReadNextBlock()
{
    mtf.ResetDecoder();

    randomized = decoder.NextSymbol(&initial_model);
    transform_index = decoder.NextWord(&initial_model, block_bits);
    num_bytes = 0;

    while (true) {
        int sel = decoder.NextSymbol(&selector_model);
        if (sel == 0 || sel == 1) { // Zero counting.
            int zero_state = 1, zero_count = 0;
            while (sel < 2) {
                if (sel == 0) zero_count += zero_state;
                else if (sel == 1) zero_count += (2 * zero_state);
                zero_state *= 2;
                sel = decoder.NextSymbol(&selector_model);
            }

            if (num_bytes + zero_count > block_size)
                throw ExtractException("Arsenic: invalid block [zero count]");

            memset(&block[num_bytes], mtf.Decode(0), zero_count);
            num_bytes += zero_count;
        }

        int symbol;
        if (sel == 10) break;
        else if (sel == 2) symbol = 1;
        else symbol = decoder.NextSymbol(&mtf_model[sel - 3]);

        if (num_bytes >= block_size)
            throw ExtractException("Arsenic: invalid block [num of bytes]");
        block[num_bytes++] = mtf.Decode(symbol);
    }

    if (transform_index >= num_bytes)
        throw ExtractException("Arsenic: invalid block [transform index]");

    selector_model.ResetModel();
    for (int i = 0; i < 7;i++)
        mtf_model[i].ResetModel();

    if (decoder.NextSymbol(&initial_model)) { // End marker.
        compcrc = decoder.NextWord(&initial_model, 32);
        end_of_blocks = true;
    }

    transform = std::make_unique<uint32_t[]>(num_bytes);
    CalculateInverseBWT(transform.get(), block.get(), num_bytes);
}



// Read the next byte.
int32_t ArsenicMethod::ReadNextByte()
{
    int byte, out_byte;

    if (repeat) {
        out_byte = last; repeat--;
        goto end;
    }

retry:
    if (byte_count >= num_bytes) {
        if (end_of_blocks) return -1;

        ReadNextBlock();
        byte_count = 0; count = 0; last = 0;
        rand_index = 0; rand_count = kRandomizationTable[0];
    }

    transform_index = transform[transform_index];
    byte = block[transform_index];

    if (randomized && rand_count == byte_count) {
        byte ^= 1;
        rand_index = (rand_index + 1) & 0xFF;
        rand_count += kRandomizationTable[rand_index];
    }

    byte_count++;

    if (count == 4) {
        count = 0;
        if (byte == 0) goto retry;
        repeat = byte - 1;
        out_byte = last;
    }
    else {
        if (byte == last) count++;
        else { count = 1; last = byte; }
        out_byte = byte;
    }

end:
    crc = CalcCRC(crc, out_byte, CRCTable_edb88320);
    return out_byte;
}



// Read the next bytes.
int32_t ArsenicMethod::ReadBytes(uint8_t *buffer, uint32_t length)
{
    if (end_of_blocks) {
        // if (compcrc != ~crc)  // FIX ME
        //     throw ExtractException("Arsenic: invalid CRC after uncompressing");
        return -1;
    }

    uint8_t *start = buffer;
    uint8_t *end_capacity = buffer + length;

    int32_t byte;
    while (buffer != end_capacity && (byte = ReadNextByte()) != -1)
        *(buffer++) = byte;

    return buffer - start;
}



} // namespace stuffit
} // namespace maconv
