#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <jpeglib.h>
#include <memory>
#include <png.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <regex>

#include "EtcImage.h"
#include "EtcColorFloatRGBA.h"

// platform specific
#include <dirent.h>
#include <sys/stat.h>



union Color
{
	struct { uint8_t r, g, b, a; } c;
	uint8_t a[4];
	uint32_t l;
};

static_assert(sizeof(Color) == sizeof(uint32_t), "Color union is the wrong size.");

// The modes we support
#define GL_COMPRESSED_RGB8_ETC2           0x9274
#define GL_COMPRESSED_SRGB8_ETC2          0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC      0x9278
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279

class Image
{
public:
	Image(const std::string& path):
		m_path(path)
	{
		std::ifstream in(path, std::ios::binary);
		if (!in)
			throw std::runtime_error("Unable to open " + path);

		in.seekg(0, std::ios::end);
		size_t file_size = in.tellg();
		in.seekg(0, std::ios::beg);

		std::vector<char> buffer(file_size);
		in.read(buffer.data(), file_size);
		in.close();

		std::string extlower;
		for (size_t i = path.find_last_of('.'); i < path.size(); ++i)
			extlower += std::tolower(path[i]);

		if (extlower == ".png")
		{
			if (!ReadPNG(buffer))
			{
				m_pixels.clear();
				throw std::runtime_error("Unable to read " + path);
			}
		}
		else if (extlower == ".jpg" || extlower == ".jpeg")
		{
			if (!ReadJPG(buffer))
			{
				m_pixels.clear();
				throw std::runtime_error("Unable to read " + path);
			}
		}
		else
		{
			throw std::runtime_error("Unknown file format for " + path);
		}
	}

	bool Valid() const { return !m_pixels.empty(); }

	int Width() const { return m_width; }
	int Height() const { return m_height; }

	const std::string& Path() const { return m_path; }

	// Color& at(int x, int y) { return m_pixels[x + y * m_height]; }
	std::vector<Color>& Pixels() { return m_pixels; }
	const std::vector<Color>& Pixels() const  { return m_pixels; }

	void Reduce(int factor)
	{
		if (factor < 2 || factor > 10)
			return;
		int new_width = m_width / factor;
		int new_height = m_height / factor;
		std::vector<Color> new_pixels(new_width * new_height);

		for (int ny = 0; ny < new_height; ++ny)
		{
			for (int nx = 0; nx < new_width; ++nx)
			{
				int r = 0;
				int g = 0;
				int b = 0;
				int a = 0;
				int count = 0;
				for (int dy = 0; dy < factor; ++dy)
				{
					for (int dx = 0; dx < factor; ++dx)
					{
						int x = nx * factor + dx;
						int y = ny * factor + dy;
						if (x < m_width && y < m_height)
						{
							Color& pixel = m_pixels[y * m_width + x];
							r += pixel.c.r;
							g += pixel.c.g;
							b += pixel.c.b;
							a += pixel.c.a;
							++count;
						}
					}
				}
				Color& new_pixel = new_pixels[ny * new_width + nx];
				new_pixel.c.r = r / count;
				new_pixel.c.g = g / count;
				new_pixel.c.b = b / count;
				new_pixel.c.a = a / count;
			}
		}

		m_width = new_width;
		m_height = new_height;
		m_pixels.swap(new_pixels);
	}

	static uint64_t Checksum(const std::string& path)
	{
		std::ifstream in(path, std::ios::binary);
		if (!in)
			throw std::runtime_error("Unable to open " + path);

		// TODO: Use a real checksum?
		uint64_t checksum = 0;
		uint64_t value = 0;
		while (in.read((char*)&value, sizeof(value)))
			checksum ^= value;
		return checksum;
	}

protected:
	bool ReadPNG(const std::vector<char>& data)
	{
		// Set up libpng.
		png_struct *png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if(!png)
			return false;

		png_info *info = png_create_info_struct(png);
		if(!info)
		{
			png_destroy_read_struct(&png, nullptr, nullptr);
			return false;
		}

		if(setjmp(png_jmpbuf(png)))
		{
			png_destroy_read_struct(&png, &info, nullptr);
			return false;
		}

		struct MemBuffer {
			const std::vector<char>& data;
			size_t pos;
		} pngData {
			data,
			0
		};
		png_set_read_fn(png, &pngData, [](png_struct* png, png_bytep data, size_t length) {
			MemBuffer* p = reinterpret_cast<MemBuffer*>(png_get_io_ptr(png));
			if (length + p->pos > p->data.size())
			{
				png_error(png, "EOF hit when reading bytes from png file");
			}
			memcpy(data, p->data.data() + p->pos, length);
			p->pos += length;
		});
		png_set_sig_bytes(png, 0);

		png_read_info(png, info);
		m_width = png_get_image_width(png, info);
		m_height = png_get_image_height(png, info);
		// If the buffer is not yet allocated, allocate it.
		m_pixels.resize(m_width * m_height);

		// Adjust settings to make sure the result will be a RGBA file.
		int colorType = png_get_color_type(png, info);
		int bitDepth = png_get_bit_depth(png, info);

		if(colorType == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png);
		if(png_get_valid(png, info, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png);
		if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
			png_set_expand_gray_1_2_4_to_8(png);
		if(bitDepth == 16)
		{
#if PNG_LIBPNG_VER >= 10504
			png_set_scale_16(png);
#else
			png_set_strip_16(png);
#endif
		}
		if(bitDepth < 8)
			png_set_packing(png);
		if(colorType == PNG_COLOR_TYPE_PALETTE || colorType == PNG_COLOR_TYPE_RGB
				|| colorType == PNG_COLOR_TYPE_GRAY)
			png_set_add_alpha(png, 0xFFFF, PNG_FILLER_AFTER);
		if(colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png);
		// Let libpng handle any interlaced image decoding.
		png_set_interlace_handling(png);
		png_read_update_info(png, info);

		// Read the file.
		std::vector<png_byte *> rows(m_height, nullptr);
		for(int y = 0; y < m_height; ++y)
			rows[y] = reinterpret_cast<png_byte *>(m_pixels.data() + y * m_width);

		png_read_image(png, &rows.front());

		// Clean up. The file will be closed automatically.
		png_destroy_read_struct(&png, &info, nullptr);

		return true;
	}

	bool ReadJPG(const std::vector<char>& jpg_data)
	{
		jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
		jpeg_create_decompress(&cinfo);
#pragma GCC diagnostic pop


		jpeg_mem_src(&cinfo, reinterpret_cast<const unsigned char*>(jpg_data.data()), jpg_data.size());
		jpeg_read_header(&cinfo, true);
		cinfo.out_color_space = JCS_EXT_RGBA;

		jpeg_start_decompress(&cinfo);
		m_width = cinfo.image_width;
		m_height = cinfo.image_height;

		m_pixels.resize(m_width * m_height);

		// Read the file.
		std::vector<JSAMPLE *> rows(m_height, nullptr);
		for(int y = 0; y < m_height; ++y)
			rows[y] = reinterpret_cast<png_byte *>(m_pixels.data() + y * m_width);

		int height = m_height;
		while(height)
			height -= jpeg_read_scanlines(&cinfo, &rows.front() + cinfo.output_scanline, height);

		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		return true;
	}

private:
	const std::string m_path;
	std::vector<Color> m_pixels;
	int m_width = 0;
	int m_height = 0;
};

class KtxFile
{
public:
	enum PremultiplyMode {PREMULTIPLY, PREMULTIPLY_DIV_4, ADDITIVE, NONE};

	KtxFile(const std::string& path, Etc::Image::Format format, PremultiplyMode mode):
		m_format{format},
		m_mode{mode}
	{
		m_out.open(path, std::ios::binary);
		switch (format)
		{
		case Etc::Image::Format::RGB8:
			m_header.internal_format = GL_COMPRESSED_RGB8_ETC2;
			m_header.base_internal_format = 0x1907; // GL_RGB
			break;
		case Etc::Image::Format::RGBA8:
			m_header.internal_format = GL_COMPRESSED_RGBA8_ETC2_EAC;
			m_header.base_internal_format = 0x1908; // GL_RGBA
			break;
		case Etc::Image::Format::RGB8A1:
			m_header.internal_format = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
			m_header.base_internal_format = 0x1908; // GL_RGBA
			break;
		case Etc::Image::Format::SRGB8:
			m_header.internal_format = GL_COMPRESSED_SRGB8_ETC2;
			m_header.base_internal_format = 0x1907; // GL_RGB
			break;
		case Etc::Image::Format::SRGBA8:
			m_header.internal_format = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
			m_header.base_internal_format = 0x1908; // GL_RGBA
			break;
		case Etc::Image::Format::SRGB8A1:
			m_header.internal_format = GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;
			m_header.base_internal_format = 0x1908; // GL_RGBA
			break;
		default:
			throw std::runtime_error("Unsupported image format");
		}
		m_out.write(reinterpret_cast<char*>(&m_header), sizeof(m_header));
	}
	~KtxFile()
	{
		// the header will have its counters updated, rewrite it.
		m_out.seekp(0);
		m_out.write(reinterpret_cast<char*>(&m_header), sizeof(m_header));
		m_out.seekp(sizeof(m_header) + m_header.key_value_data);
		m_out.write(reinterpret_cast<char*>(&m_image_size), sizeof(m_image_size));
	}

	void AddKeyValue(const std::string& key, const std::string& value)
	{
		if (m_image_size)
			throw std::runtime_error("Can't add key-value data after images have been added");

		// Key is a null-terminated string. Value is arbitrary binary data, but
		// encouraged to be a null-terminated string as well
		uint32_t kv_size = key.size() + 1 + value.size() + 1;
		// the padding totals are not included in the individual kv header, but
		// *is* included as part of the file header
		uint32_t kv_padding = 3 - (kv_size + 3) % 4;
		m_header.key_value_data += sizeof(uint32_t) + kv_size + kv_padding;

		m_out.write(reinterpret_cast<char*>(&kv_size), sizeof(kv_size)); // not including padding
		m_out.write(key.c_str(), key.size() + 1); // include null character
		m_out.write(value.c_str(), value.size() + 1); // include null character
		char padding_bytes[4] = {0, 0, 0, 0};
		m_out.write(padding_bytes, kv_padding);
	}

	void AddImage(const Image& i)
	{
		if (m_header.width == 0 && m_header.height == 0)
		{
			m_header.width = i.Width();
			m_header.height = i.Height();
		}
		else if (i.Width() != m_header.width || i.Height() != m_header.height)
		{
			throw std::runtime_error("All images must be the same width and height: " + i.Path() +
				" (" + std::to_string(m_header.width) + ", " + std::to_string(m_header.height) + ") != " +
				" (" + std::to_string(i.Width()) + ", " + std::to_string(i.Height()) + ")");
		}

		// Premultiply the pixels and convert them to floating point
		std::vector<Etc::ColorFloatRGBA> img_data;
		img_data.reserve(i.Width() * i.Height() * 4);
		for (const auto& c: i.Pixels())
		{
			Etc::ColorFloatRGBA rgba;

			if (m_mode == NONE)
			{
				rgba = Etc::ColorFloatRGBA::ConvertFromRGBA8(c.c.r, c.c.g, c.c.b, c.c.a);
			}
			else
			{

				// for all premultiply modes, go ahead and multiply the color
				// components by alpha/255, then convert to floating point by
				// dividing by 255 again to get it to a 0.0 to 1.0 range
				rgba.fR = static_cast<unsigned int>(c.c.r) * c.c.a / 255.0 / 255.0;
				rgba.fG = static_cast<unsigned int>(c.c.g) * c.c.a / 255.0 / 255.0;
				rgba.fB = static_cast<unsigned int>(c.c.b) * c.c.a / 255.0 / 255.0;

				if (m_mode == ADDITIVE)
				{
					// For additive, set alpha to zero, which means that new pixels
					// drawn on top of it are not diminished (ie, purely additive)
					rgba.fA = 0;
				}
				else if (m_mode == PREMULTIPLY_DIV_4)
				{
					// I have no clue what the intended effect is here. It would
					// have an effect somewhere between normal blending and additive
					// blending.
					rgba.fA = c.c.a / 4. / 255.0;
				}
				else if (m_mode == PREMULTIPLY)
				{
					// Normal pre-multiplied alpha. just convert the alpha to float
					rgba.fA = c.c.a / 255.0;
				}
			}

			img_data.push_back(rgba);
		}

		// Compress data
		Etc::Image image(reinterpret_cast<float*>(img_data.data()), i.Width(), i.Height(), Etc::ErrorMetric::RGBX);
		int thread_count = std::thread::hardware_concurrency() - 1;
		if (thread_count < 1)
			thread_count = 1;
		image.Encode(m_format, Etc::ErrorMetric::RGBX, 100, thread_count, thread_count);

		// pack data into ktx file
		AddRawData(image.GetEncodingBits(), image.GetEncodingBitsBytes());
	}

	static bool MatchesChecksum(uint64_t checksum, const std::string& path, size_t frames, Etc::Image::Format format)
	{
		std::ifstream in(path, std::ios::binary);
		if (!in)
			return false;
		KtxHeader header{};
		if (in.read((char*)&header, sizeof(header)).gcount() != sizeof(header))
			return false;

		static constexpr uint8_t KTX_MAGIC[] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
		if (0 != memcmp(KTX_MAGIC, header.magic, sizeof(KTX_MAGIC)))
			return false;

		if (header.swap != 0x04030201)
			return false;

		if (header.format != 0)
			return false;

		if (header.mipmaps > 1) return false;
		if (header.faces > 1) return false;
		if (header.depth > 1) return false;
		if (header.array_elements != frames) return false;

		switch(format)
		{
		case Etc::Image::Format::RGB8:
			if (header.internal_format != GL_COMPRESSED_RGB8_ETC2) return false;
			if (header.base_internal_format != 0x1907) return false; // GL_RGB
			break;
		case Etc::Image::Format::RGBA8:
			if (header.internal_format != GL_COMPRESSED_RGBA8_ETC2_EAC) return false;
			if (header.base_internal_format != 0x1908) return false; // GL_RGBA
			break;
		case Etc::Image::Format::RGB8A1:
			if (header.internal_format != GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2) return false;
			if (header.base_internal_format != 0x1908) return false; // GL_RGBA
			break;
		case Etc::Image::Format::SRGB8:
			if (header.internal_format != GL_COMPRESSED_SRGB8_ETC2) return false;
			if (header.base_internal_format != 0x1907) return false; // GL_RGB
			break;
		case Etc::Image::Format::SRGBA8:
			if (header.internal_format != GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC) return false;
			if (header.base_internal_format != 0x1908) return false; // GL_RGBA
			break;
		case Etc::Image::Format::SRGB8A1:
			if (header.internal_format != GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2) return false;
			if (header.base_internal_format != 0x1908) return false; // GL_RGBA
			break;
		default:
			return false;
		}

		if (header.key_value_data == 0)
			return false;

		// read key/value pairs
		std::vector<char> data(header.key_value_data);
		if (in.read(data.data(), data.size()).gcount() != data.size()) return false;
		const char* p = data.data();
		const char* pend = data.data() + data.size();
		while (p + sizeof(uint32_t) < pend)
		{
			uint32_t kv_size = *reinterpret_cast<const uint32_t*>(p);
			if (p + kv_size > pend)
				return false;
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

				if (key == "source_checksum")
				{
					try
					{
						if (checksum == std::stoull(value))
							return true;
					}
					catch(...)
					{
						return false;
					}
				}

				p = kvend;
				p += 3 - ((kv_size + 3) % 4); // padding
			}
		}
		// Didn't find the embedded checksum.
		return false;
	}

protected:

	void AddRawData(const void* data, size_t size)
	{
		++m_header.array_elements;
		if (m_image_size == 0)
		{
			// we haven't written this to the file yet, so save space for it.
			// write the updated value in the destructor.
			m_out.write(reinterpret_cast<char*>(&m_image_size), sizeof(m_image_size));
		}
		m_image_size += size;
		m_out.write(reinterpret_cast<const char*>(data), size);
	}

private:
	struct KtxHeader
	{
		uint8_t     magic[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
		uint32_t    swap = 0x04030201;
		uint32_t    type = 0;
		uint32_t    type_size = 0;
		uint32_t    format = 0;
		uint32_t    internal_format = 0;
		uint32_t    base_internal_format = 0;
		uint32_t    width = 0;
		uint32_t    height = 0;
		uint32_t    depth = 0;
		uint32_t    array_elements = 0;
		uint32_t    faces = 0;
		uint32_t    mipmaps = 0;
		uint32_t    key_value_data = 0;
	} m_header;

	// This is the mipmap header, but we only support one layer
	uint32_t m_image_size = 0;


	std::ofstream m_out;
	Etc::Image::Format m_format;
	PremultiplyMode m_mode;
};

class EndlessTextures
{
public:
	void ReadImageDirectory(std::string path, int prefix_length = -1)
	{
		// TODO: cross platform?
		if (path.empty())
			return;

		if (path.back() != '/')
			path.push_back('/');

		if (prefix_length == -1)
			prefix_length = path.size();

		DIR *dir = opendir(path.c_str());
		if(!dir)
		{
			return;
		}
		dirent* ent = nullptr;

		while((ent = readdir(dir)))
		{
			// Skip dotfiles (including "." and "..").
			if(ent->d_name[0] == '.')
				continue;

			std::string name = path + ent->d_name;
			struct stat buf;
			stat(name.c_str(), &buf);

			if (S_ISDIR(buf.st_mode))
			{
				ReadImageDirectory(name, prefix_length);
			}
			else
			{
				// endless sky uses a naming convention that looks like this:
				// file[blendmode[frame_index]], where blendmode looks like:
				//    + Additive blending mode
				//    ^ in-between blending mode (It used tobe ~)
				//    - normal blending mode (the default if absent)
				//    = premultiplied blending mode
				// If the blending mode is absent, then any number is just part
				// of the name.
				static const std::regex r("(.*?)(([^\\s\\w\\(\\)])\\d*?)?(\\.png|\\.jpg)", std::regex_constants::icase);
				std::smatch match;
				if (std::regex_match(name, match, r))
				{
					++m_file_count;
					auto& ktx = m_ktx_files[match[1].str().substr(prefix_length)];
					ktx.frames.push_back(name);
					if (match[3].length())
					{
						if (match[3] == '-')
							ktx.mode = KtxFile::PREMULTIPLY;
						else if (match[3] == '+')
							ktx.mode = KtxFile::ADDITIVE;
						else if (match[3] == '~' || match[3] == '^')
							ktx.mode = KtxFile::PREMULTIPLY_DIV_4;
						else if (match[3] == '=')
							ktx.mode = KtxFile::NONE;
						else
							throw std::runtime_error("Unsupported premultiply mode for " + name);
					}
					else
					{
						ktx.mode = KtxFile::PREMULTIPLY;
					}

					if (match[4] == ".jpg" && (ktx.mode == KtxFile::NONE || ktx.mode == KtxFile::PREMULTIPLY))
					{
						// jpeg files have no alpha channel, so if we aren't using
						// a special blending mode, just use RGB8 since it takes
						// half the space
						ktx.format = Etc::Image::Format::RGB8;
					}
					else
					{
						ktx.format = Etc::Image::Format::RGBA8;
						// TODO: Auto-detect RGB8A1 use cases?
						//       I don't think we have any though.
					}
				}
			}
		}

		closedir(dir);
	}

	void DumpDebugInfo(const std::string& output_path)
	{
		for (auto& kv: m_ktx_files)
		{
			std::string target_file = output_path + "/" + kv.first + "=.ktx";
			std::string target_path = target_file.substr(0, target_file.rfind('/'));
			switch (kv.second.mode)
			{
			case KtxFile::NONE: 		         std::cout << "= "; break;
			case KtxFile::ADDITIVE:          std::cout << "+ "; break;
			case KtxFile::PREMULTIPLY:       std::cout << "- "; break;
			case KtxFile::PREMULTIPLY_DIV_4: std::cout << "~ "; break;
			}

			switch(kv.second.format)
			{
			case Etc::Image::Format::RGB8:   std::cout << "RGB8   "; break;
			case Etc::Image::Format::RGBA8:  std::cout << "RGBA8  "; break;
			case Etc::Image::Format::RGB8A1: std::cout << "RGB8A1 "; break;
			default:                         std::cout << "?????? "; break;
			}
			std::cout << target_file << "\n";
			std::sort(kv.second.frames.begin(), kv.second.frames.end());
			for (auto& f: kv.second.frames)
			{
				std::cout << "   " << f << '\n';
			}
		}
	}

	void WriteTextures(const std::string& output_path, int reduce_factor)
	{
		int done_count = 0;
		int skipped_count = 0;
		std::cout << "[0/" << m_file_count << "]" << std::flush;
		for (auto& kv: m_ktx_files)
		{
			std::string target_file = output_path + "/" + kv.first + "=.ktx";
			std::string target_path = target_file.substr(0, target_file.rfind('/'));
			if (!mkdir_p(target_path))
			{
				throw std::runtime_error("Unable to create " + target_path + " directory");
			}
			std::sort(kv.second.frames.begin(), kv.second.frames.end());

			uint64_t checksum = 0;
			for (auto& frame: kv.second.frames)
				checksum ^= Image::Checksum(frame);

			if (KtxFile::MatchesChecksum(checksum, target_file, kv.second.frames.size(), kv.second.format))
			{
				skipped_count += kv.second.frames.size();
			}
			else
			{
				auto it = kv.second.frames.begin();
				Image img(*it);
				int original_width = img.Width();
				int original_height = img.Height();
				bool do_reduce = false;
				// Don't reduce ui or menu panels, or the fonts. They look like
				// garbage if we mess with them. Explicitly reduce the haze though
				if (kv.first.substr(0, 10) == "_menu/haze")
				{
					if (reduce_factor > 1 && img.Width() > 8 && img.Height() > 8)
					{
						img.Reduce(reduce_factor);
						do_reduce = true;
					}
				}
				else if(kv.first.substr(0, 3) != "ui/" &&
					kv.first.substr(0, 6) != "_menu/" &&
					kv.first.substr(0, 5) != "font/")
				{
					if (reduce_factor > 1 && img.Width() * img.Height() > 250000)
					{
						img.Reduce(reduce_factor);
						do_reduce = true;
					}
				}
				KtxFile ktx(target_file, kv.second.format, kv.second.mode);
				ktx.AddKeyValue("original_width", std::to_string(original_width));
				ktx.AddKeyValue("original_height", std::to_string(original_height));
				ktx.AddKeyValue("source_checksum", std::to_string(checksum));
				ktx.AddImage(img);
				++done_count;
				for (++it; it != kv.second.frames.end(); ++it)
				{
					Image img(*it);
					if (do_reduce)
						img.Reduce(reduce_factor);
					ktx.AddImage(img);
					++done_count;
				}
			}
			std::cout << "\r[" << done_count << "+" << skipped_count << "/" << m_file_count << "]" << std::flush;
		}
		std::cout << '\n';
	}

private:

	bool mkdir_p(const std::string path)
	{
		size_t next = path.find_first_of("/\\");
		size_t pos = 0;
		std::string built_path;
		if (!path.empty() && path.front() != '/')
		{
			built_path = '.';
		}
		std::vector<std::string> to_create;
		for(;;)
		{
			std::string component = (next == path.npos) ? path.substr(pos)
																	: path.substr(pos, next - pos);

			if (!component.empty() && component != ".")
			{
				if (component == "..")
				{
					return false; // not doing this
				}
				built_path += "/" + component;
				struct stat buf;
				if (stat(built_path.c_str(), &buf) == 0)
				{
					if (!S_ISDIR(buf.st_mode))
					{
						// it exists, and its not a directory. can't continue
						return false;
					}
				}
				else
				{
					// doesn't exist. add it
					to_create.push_back(built_path);
				}
			}
			if (next == std::string::npos)
			{
				break;
			}
			pos = next + 1; //discard slash
			next = path.find_first_of("/\\", pos);
		}
		for (const std::string& component: to_create)
		{
			if (mkdir(component.c_str(), 0700) != 0)
			{
				return false;
			}
		}
		return true;
	}

	struct KtxComposition
	{
		KtxFile::PremultiplyMode mode;
		Etc::Image::Format format;
		std::vector<std::string> frames;
	};

	std::map<std::string, KtxComposition> m_ktx_files;

	int m_file_count = 0;

	std::thread m_thread;
};

int main(int argc, char** argv)
{
	std::vector<std::unique_ptr<Image>> frames;
	std::string output_path;
	std::string batch_path;
	int reduce_factor = 0;

	KtxFile::PremultiplyMode mode = KtxFile::NONE;

	Etc::Image::Format compression_mode = Etc::Image::Format::RGBA8;

	bool help_and_exit = false;
	bool debug = false;

	for (int i = 1; i < argc; ++i)
	{
		if (0 == strcmp(argv[i], "--premultiply") ||
		    0 == strcmp(argv[i], "-p"))
		{
			// normal premultiply
			mode = KtxFile::PREMULTIPLY;
		}
		else if (0 == strcmp(argv[i], "--premultiply_div_4") ||
		         0 == strcmp(argv[i], "-d"))
		{
			// I don't understand this mode, but some files premultiply, and
			// then divide the alpha by 4 before adding it back in.
			mode = KtxFile::PREMULTIPLY_DIV_4;
		}
		else if (0 == strcmp(argv[i], "--additive") ||
		         0 == strcmp(argv[i], "-a"))
		{
			// premultiply, then remove the alpha
			mode = KtxFile::ADDITIVE;
		}
		else if (0 == strcmp(argv[i], "--rgb") ||
		         0 == strcmp(argv[i], "-3"))
		{
			// 3 bytes per pixel, no alpha
			compression_mode = Etc::Image::Format::RGB8;
		}
		else if (0 == strcmp(argv[i], "--rgba1") ||
		         0 == strcmp(argv[i], "-4"))
		{
			// 3 bytes per pixel, binary bitmask alpha
			compression_mode = Etc::Image::Format::RGB8A1;
		}
		else if (0 == strcmp(argv[i], "--rgba") ||
		         0 == strcmp(argv[i], "-5"))
		{
			// 4 bytes per pixel, including alpha
			compression_mode = Etc::Image::Format::RGBA8;
		}
		else if (0 == strcmp(argv[i], "--output") ||
		         0 == strcmp(argv[i], "-o"))
		{
			if (i + 1 < argc)
			{
				output_path = argv[i+1];
				++i;
			}
			else
			{
				std::cerr << "--output requires a filename argument\n";
				help_and_exit = true;
				break;
			}
		}
		else if (0 == strcmp(argv[i], "--batch") ||
		         0 == strcmp(argv[i], "-b"))
		{
			if (i + 1 < argc)
			{
				batch_path = argv[i+1];
				++i;
			}
			else
			{
				std::cerr << "--batch requires a path argument\n";
				help_and_exit = true;
				break;
			}
		}
		else if (0 == strcmp(argv[i], "--reduce") ||
		         0 == strcmp(argv[i], "-r"))
		{
			if (i + 1 < argc)
			{
				reduce_factor = std::atoi(argv[i+1]);
				++i;
			}
			else
			{
				std::cerr << "--reduce requires a factor argument\n";
				help_and_exit = true;
				break;
			}
		}
		else if (0 == strcmp(argv[i], "--debug") ||
		         0 == strcmp(argv[i], "-d"))
		{
			debug = true;
		}
		else if (0 == strcmp(argv[i], "--help") ||
		         0 == strcmp(argv[i], "-h"))
		{
			help_and_exit = true;
			break;
		}
		else
		{
			// assume a filename
			std::unique_ptr<Image> si(new Image(argv[i]));
			if (!si->Valid())
			{
				std::cerr << "Unable to read " << argv[i] << '\n';
				return 1;
			}
			if (!frames.empty())
			{
				// validate all files are the same size
				if (si->Width() != frames.front()->Width() ||
				    si->Height() != frames.front()->Height())
				{
					std::cerr << "All input files must have the same width and height\n";
					help_and_exit = true;
					break;
				}
			}
			frames.emplace_back(std::move(si));
		}
	}
	if (!batch_path.empty())
	{
		struct stat st;
		if (0 != stat(batch_path.c_str(), &st) || !S_ISDIR(st.st_mode))
		{
			std::cerr << "--batch input_path must point to a directory\n";
			help_and_exit = true;
		}
		if (output_path.empty())
		{
			std::cerr << "No output path specified\n";
			help_and_exit = true;
		}
	}
	else
	{
		if (output_path.empty())
		{
			std::cerr << "No output file specified\n";
			help_and_exit = true;
		}

		if (frames.empty())
		{
			std::cerr << "No input files specified\n";
			help_and_exit = true;
		}
	}

	if (reduce_factor < 0 or reduce_factor > 20)
	{
		std::cerr << "The factor passed to --reduce must be a small positive integer\n";
		help_and_exit = true;
	}

	if (help_and_exit)
	{
		std::cerr << "Usage: " << argv[0] << " [arguments] -o output_file <input files>\n";
		std::cerr << "   -p --premultiply          Normal premultiply mode\n";
		std::cerr << "   -a --additive             Additive premultiply mode\n";
		std::cerr << "   -d --premultiply_div_4    Premultiply then divide alpha by 4\n";
		std::cerr << "   -3 --rgb                  Compress using GL_COMPRESSED_RGB8_ETC2\n";
		std::cerr << "   -4 --rgba1                Compress using GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2\n";
		std::cerr << "   -5 --rgba                 Compress using GL_COMPRESSED_RGBA8_ETC2_EAC [default]\n";
		//std::cerr << "   -z --zipit                Use libz to further compress results\n";
		std::cerr << "   -o --output <output_file> Path to output file or directory\n";
		std::cerr << "   -b --batch <input_path>   Recursively batch process all files in input_path\n";
		std::cerr << "   -r --reduce <factor>      How much to reduce texture size (ui/menu exempted)\n";
		std::cerr << "   -h --help                 Output this help and exit.\n";

		return 1;
	}

	try
	{
		if (batch_path.empty())
		{
			// single file mode
			KtxFile output_file(output_path, compression_mode, mode);
			output_file.AddKeyValue("original_width", std::to_string(frames.front()->Width()));
			output_file.AddKeyValue("original_height", std::to_string(frames.front()->Height()));

			// Compress and pack all frames into a ktx file.
			for (auto& f: frames)
			{
				f->Reduce(reduce_factor);
				output_file.AddImage(*f);
			}
		}
		else
		{
			// batch process the entire folder
			EndlessTextures et;
			et.ReadImageDirectory(batch_path);
			if (debug)
				et.DumpDebugInfo(output_path);
			else
				et.WriteTextures(output_path, reduce_factor);
		}
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "\nCaught runtime exception: " << e.what() << '\n';
		return 1;
	}

	return 0;
}
