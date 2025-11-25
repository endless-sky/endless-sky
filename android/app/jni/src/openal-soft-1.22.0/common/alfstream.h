#ifndef AL_FSTREAM_H
#define AL_FSTREAM_H

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <string>
#include <fstream>


namespace al {

// Windows' std::ifstream fails with non-ANSI paths since the standard only
// specifies names using const char* (or std::string). MSVC has a non-standard
// extension using const wchar_t* (or std::wstring?) to handle Unicode paths,
// but not all Windows compilers support it. So we have to make our own istream
// that accepts UTF-8 paths and forwards to Unicode-aware I/O functions.
class filebuf final : public std::streambuf {
    std::array<char_type,4096> mBuffer;
    HANDLE mFile{INVALID_HANDLE_VALUE};

    int_type underflow() override;
    pos_type seekoff(off_type offset, std::ios_base::seekdir whence, std::ios_base::openmode mode) override;
    pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override;

public:
    filebuf() = default;
    ~filebuf() override;

    bool open(const wchar_t *filename, std::ios_base::openmode mode);
    bool open(const char *filename, std::ios_base::openmode mode);

    bool is_open() const noexcept { return mFile != INVALID_HANDLE_VALUE; }

    void close();
};

// Inherit from std::istream to use our custom streambuf
class ifstream final : public std::istream {
    filebuf mStreamBuf;

public:
    ifstream(const wchar_t *filename, std::ios_base::openmode mode = std::ios_base::in);
    ifstream(const std::wstring &filename, std::ios_base::openmode mode = std::ios_base::in)
      : ifstream(filename.c_str(), mode) { }
    ifstream(const char *filename, std::ios_base::openmode mode = std::ios_base::in);
    ifstream(const std::string &filename, std::ios_base::openmode mode = std::ios_base::in)
      : ifstream(filename.c_str(), mode) { }
    ~ifstream() override;

    bool is_open() const noexcept { return mStreamBuf.is_open(); }

    void close() { mStreamBuf.close(); }
};

} // namespace al

#else /* _WIN32 */

#include <fstream>

namespace al {

using filebuf = std::filebuf;
using ifstream = std::ifstream;

} // namespace al

#endif /* _WIN32 */

#endif /* AL_FSTREAM_H */
