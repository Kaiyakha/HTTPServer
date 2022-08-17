#pragma once
#include <cstdint>
#include <memory>

const bool mirror_jpg(const uint8_t* data, uint8_t* result, const size_t input_size, size_t& output_size);
