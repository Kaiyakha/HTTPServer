#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PNG
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include "image.hpp"

#pragma warning (disable: 6386 6385 6262 6061 4267 4244 26819 26451)


typedef struct {
	size_t last_pos;
	uint8_t* buffer;
} stbi_memory_context;

static void stbi_write_memory(void* context, void* data, int size);


const bool mirror_jpg(const uint8_t* const data, uint8_t* result, const size_t input_size, size_t& output_size) {
	int width, height, channels;
	stbi_uc* image;

	image = stbi_load_from_memory((stbi_uc*)data, static_cast<int>(input_size), &width, &height, &channels, 0);
	if (image == NULL) return false;

	const size_t res = width * height;
	std::unique_ptr<uint8_t[]> temp(new uint8_t[channels]);

	size_t pix_id, mir_pix_id, ch, byte_id, mir_byte_id;
	for (pix_id = 0; pix_id < res; pix_id++) {
		if (pix_id % width >= width >> 1) {
			--pix_id += static_cast<size_t>(std::ceil(static_cast<float>(width) / 2));
			continue;
		}
		mir_pix_id = static_cast<size_t>(std::floor(pix_id / width) + 1) * width - (pix_id % width + 1);
		byte_id = pix_id * channels;
		mir_byte_id = mir_pix_id * channels;
		for (ch = 0; ch < channels; ch++) {
			temp[ch] = image[byte_id];
			image[byte_id] = image[mir_byte_id];
			image[mir_byte_id] = temp[ch];
			byte_id++; mir_byte_id++;
		}
	}
	
	stbi_memory_context context;
	context.last_pos = 0;
	context.buffer = result;
	const int success = stbi_write_jpg_to_func(stbi_write_memory, &context, width, height, channels, image, 100);
	output_size = context.last_pos;
	
	stbi_image_free(image);

	if (success) return true;
	else return false;
}


static void stbi_write_memory(void* context, void* data, int size) {
	stbi_memory_context* pcontext = (stbi_memory_context*)context;
	uint8_t* dst = pcontext->buffer;
	uint8_t* src = (uint8_t*)data;
	size_t cur_pos = pcontext->last_pos;

	for (size_t i = 0; i < size; i++)
		dst[cur_pos++] = src[i];

	pcontext->last_pos = cur_pos;
}