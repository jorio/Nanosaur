/*

All Stuffit compression methods.

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

//#include "stuffit/stuffit.h"

#include <memory>
#include <string>
#include <stdexcept>

namespace maconv {
namespace stuffit {


// Exception when extracting a fork.
struct ExtractException : public std::runtime_error {

    ExtractException(const char* msg) : std::runtime_error(msg) {}
};



// A compression method.
struct CompressionMethod {

    virtual ~CompressionMethod() = default;


    // Read the next bytes.
    virtual int32_t ReadBytes(uint8_t *data, uint32_t length) { return -1; }

    // Initialize the algorithm.
    virtual void Initialize() {}


    // Extract data from the compressed fork.
//    virtual void Extract(const StuffitCompInfo &info, uint8_t *data,
//        std::vector<fs::DataPtr> &mem_pool);

    uint8_t *data; // Compressed data.
    uint8_t *end; // End of compressed data.

    uint8_t *uncompressed; // Pointer on t uncompressed data.
    uint32_t total_size; // Length of uncompressed data.
};



// An unique pointer on a compression method.
using CompMethodPtr = std::unique_ptr<CompressionMethod>;

// Get a compression method (from a method number).
CompMethodPtr GetCompressionMethod(uint8_t method);



// "No compression" method.
struct NoneMethod : CompressionMethod {

    // Extract data from the compressed fork.
//    void Extract(const StuffitCompInfo &info, uint8_t *data,
//        std::vector<fs::DataPtr> &mem_pool) override;
};


} // namespace stuffit
} // namespace maconv
