#pragma once
#include <string>

/**
 *  Reads a compressed texture from a texture file.
 */
class KtxFile
{
public:
	KtxFile(const std::string& src_data);

	bool Valid() const { return header != nullptr; }

	uint32_t InternalFormat() const;
	uint32_t Width() const;
	uint32_t Height() const;
	uint32_t OriginalWidth() const {return original_width; }
	uint32_t OriginalHeight() const {return original_height; }
	uint32_t Frames() const;
	uint32_t Size() const;

	const void* Data() const;

private:
	const struct ktx_header* header = nullptr;
	uint32_t original_width = 0;
	uint32_t original_height = 0;
};