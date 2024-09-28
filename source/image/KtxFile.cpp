#include "KtxFile.h"

#include "../opengl.h"

#include <cstdint>
#include <cstring>

struct ktx_header
{
	uint8_t     magic[12];
	uint32_t    swap;
	uint32_t    type;
	uint32_t    type_size;
	uint32_t    format;
	uint32_t    internal_format;
	uint32_t    base_internal_format;
	uint32_t    width;
	uint32_t    height;
	uint32_t    depth;
	uint32_t    array_elements;
	uint32_t    faces;
	uint32_t    mipmaps;
	uint32_t    key_value_data;
};

KtxFile::KtxFile(const std::string& data)
{
	if (data.size() < sizeof(ktx_header))
		return;

	const ktx_header* header = reinterpret_cast<const ktx_header*>(data.data());

	static const uint8_t ktx_identifier[] = {
		0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
	};

	if (0 != memcmp(ktx_identifier, header->magic, sizeof(ktx_identifier)))
		return;

	if (header->swap != 0x04030201)
		return;
	// else TODO: add swaps. The actual texture data is endian-independent

	// this is the glFormat passed to glTexImage. For compressed textures, it
	// will be zero
	if (header->format != 0) return;

	// internal format is the argument passed to glCompressedTexImage. Only
	// allow ETC2 compression here (its support is required by gles 3)
	switch(header->internal_format)
	{
	case GL_COMPRESSED_RGB8_ETC2:
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
		break;
	default: return;
	}

	// only allow RGB/RGBA here
	switch (header->base_internal_format)
	{
	case GL_RGBA:
	case GL_RGB:
		break;
	default: return;
	}

	// Don't support mipmaps, faces, or depth
	if (header->mipmaps > 1) return;
	if (header->faces > 1) return;
	if (header->depth > 1) return;

	// Do support arrays, as this is how we handle animation frames.

	// Validate that we have as much data as it claims we do.
	ssize_t remaining_size = data.size() - sizeof(ktx_header) - header->key_value_data;
	if (remaining_size < static_cast<ssize_t>(sizeof(uint32_t))) return;
	uint32_t image_size = *reinterpret_cast<const uint32_t*>(data.data() + sizeof(ktx_header) + header->key_value_data);
	if (remaining_size - image_size < 0) return;

	// Default original_width/original_height, which may be overridden by the
	// key/value data
	original_width = header->width;
	original_height = header->height;

	// process key/value pairs
	const char* p = data.data() + sizeof(ktx_header);
	const char* pend = p + header->key_value_data;
	while (p + sizeof(uint32_t) < pend)
	{
		uint32_t kv_size = *reinterpret_cast<const uint32_t*>(p);
		if (p + kv_size > pend)
			return;
		p += sizeof(uint32_t);
		const char* kvend = p + kv_size;

		// key is null-terminated string
		std::string key;
		while (p < pend && *p) key += *p++;

		if (p < pend)
		{
			++p;
			// value is arbitrary data, but likely is also
			// a null terminated string. constructing it this way preserves the
			// null within the string (it likely has two nulls, not one)
			std::string value(p, kvend);

			if (key == "original_width")
			{
				uint32_t i = atoi(value.c_str());
				if (i)
					original_width = i;
			}
			else if (key == "original_height")
			{
				uint32_t i = atoi(value.c_str());
				if (i)
					original_height = i;
			}

			p = kvend;
			p += 3 - ((kv_size + 3) % 4); // padding
		}
	}

	// At this point, the file is probably valid.
	this->header = header;
}



uint32_t KtxFile::InternalFormat() const
{
	return header->internal_format;
}

uint32_t KtxFile::Width() const
{
	// TODO: account for padding?
	return header->width;
}

uint32_t KtxFile::Height() const
{
	// TODO: account for padding?
	return header->height;
}

uint32_t KtxFile::Frames() const
{
	// array_elements == 0 means this isn't an array texture, but we still treat
	// it like one, so mark it as having 1 frame.
	return header->array_elements ? header->array_elements : 1;
}

uint32_t KtxFile::Size() const
{
	const uint8_t* p = reinterpret_cast<const uint8_t*>(header + 1);
	p += header->key_value_data;
	return *reinterpret_cast<const uint32_t*>(p);
}

const void* KtxFile::Data() const
{
	const uint8_t* p = reinterpret_cast<const uint8_t*>(header + 1);
	p += header->key_value_data;
	p += sizeof(uint32_t);
	return p;
}