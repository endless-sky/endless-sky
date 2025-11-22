
#include "config.h"

#include "alfstream.h"

#include "strutils.h"

#ifdef _WIN32

namespace al {

auto filebuf::underflow() -> int_type
{
    if(mFile != INVALID_HANDLE_VALUE && gptr() == egptr())
    {
        // Read in the next chunk of data, and set the pointers on success
        DWORD got{};
        if(ReadFile(mFile, mBuffer.data(), static_cast<DWORD>(mBuffer.size()), &got, nullptr))
            setg(mBuffer.data(), mBuffer.data(), mBuffer.data()+got);
    }
    if(gptr() == egptr())
        return traits_type::eof();
    return traits_type::to_int_type(*gptr());
}

auto filebuf::seekoff(off_type offset, std::ios_base::seekdir whence, std::ios_base::openmode mode) -> pos_type
{
    if(mFile == INVALID_HANDLE_VALUE || (mode&std::ios_base::out) || !(mode&std::ios_base::in))
        return traits_type::eof();

    LARGE_INTEGER fpos{};
    switch(whence)
    {
        case std::ios_base::beg:
            fpos.QuadPart = offset;
            if(!SetFilePointerEx(mFile, fpos, &fpos, FILE_BEGIN))
                return traits_type::eof();
            break;

        case std::ios_base::cur:
            // If the offset remains in the current buffer range, just
            // update the pointer.
            if((offset >= 0 && offset < off_type(egptr()-gptr())) ||
                (offset < 0 && -offset <= off_type(gptr()-eback())))
            {
                // Get the current file offset to report the correct read
                // offset.
                fpos.QuadPart = 0;
                if(!SetFilePointerEx(mFile, fpos, &fpos, FILE_CURRENT))
                    return traits_type::eof();
                setg(eback(), gptr()+offset, egptr());
                return fpos.QuadPart - off_type(egptr()-gptr());
            }
            // Need to offset for the file offset being at egptr() while
            // the requested offset is relative to gptr().
            offset -= off_type(egptr()-gptr());
            fpos.QuadPart = offset;
            if(!SetFilePointerEx(mFile, fpos, &fpos, FILE_CURRENT))
                return traits_type::eof();
            break;

        case std::ios_base::end:
            fpos.QuadPart = offset;
            if(!SetFilePointerEx(mFile, fpos, &fpos, FILE_END))
                return traits_type::eof();
            break;

        default:
            return traits_type::eof();
    }
    setg(nullptr, nullptr, nullptr);
    return fpos.QuadPart;
}

auto filebuf::seekpos(pos_type pos, std::ios_base::openmode mode) -> pos_type
{
    // Simplified version of seekoff
    if(mFile == INVALID_HANDLE_VALUE || (mode&std::ios_base::out) || !(mode&std::ios_base::in))
        return traits_type::eof();

    LARGE_INTEGER fpos{};
    fpos.QuadPart = pos;
    if(!SetFilePointerEx(mFile, fpos, &fpos, FILE_BEGIN))
        return traits_type::eof();

    setg(nullptr, nullptr, nullptr);
    return fpos.QuadPart;
}

filebuf::~filebuf()
{ close(); }

bool filebuf::open(const wchar_t *filename, std::ios_base::openmode mode)
{
    if((mode&std::ios_base::out) || !(mode&std::ios_base::in))
        return false;
    HANDLE f{CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr)};
    if(f == INVALID_HANDLE_VALUE) return false;

    if(mFile != INVALID_HANDLE_VALUE)
        CloseHandle(mFile);
    mFile = f;

    setg(nullptr, nullptr, nullptr);
    return true;
}
bool filebuf::open(const char *filename, std::ios_base::openmode mode)
{
    std::wstring wname{utf8_to_wstr(filename)};
    return open(wname.c_str(), mode);
}

void filebuf::close()
{
    if(mFile != INVALID_HANDLE_VALUE)
        CloseHandle(mFile);
    mFile = INVALID_HANDLE_VALUE;
}


ifstream::ifstream(const wchar_t *filename, std::ios_base::openmode mode)
  : std::istream{nullptr}
{
    init(&mStreamBuf);

    // Set the failbit if the file failed to open.
    if((mode&std::ios_base::out) || !mStreamBuf.open(filename, mode|std::ios_base::in))
        clear(failbit);
}

ifstream::ifstream(const char *filename, std::ios_base::openmode mode)
  : std::istream{nullptr}
{
    init(&mStreamBuf);

    // Set the failbit if the file failed to open.
    if((mode&std::ios_base::out) || !mStreamBuf.open(filename, mode|std::ios_base::in))
        clear(failbit);
}

/* This is only here to ensure the compiler doesn't define an implicit
 * destructor, which it tries to automatically inline and subsequently complain
 * it can't inline without excessive code growth.
 */
ifstream::~ifstream() { }

} // namespace al

#endif
