/*
 * HRTF utility for producing and demonstrating the process of creating an
 * OpenAL Soft compatible HRIR data set.
 *
 * Copyright (C) 2011-2019  Christopher Fitzgerald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Or visit:  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include "loaddef.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <cstdarg>
#include <vector>

#include "alfstream.h"
#include "alstring.h"
#include "makemhr.h"

#include "mysofa.h"

// Constants for accessing the token reader's ring buffer.
#define TR_RING_BITS                 (16)
#define TR_RING_SIZE                 (1 << TR_RING_BITS)
#define TR_RING_MASK                 (TR_RING_SIZE - 1)

// The token reader's load interval in bytes.
#define TR_LOAD_SIZE                 (TR_RING_SIZE >> 2)

// Token reader state for parsing the data set definition.
struct TokenReaderT {
    std::istream &mIStream;
    const char *mName{};
    uint        mLine{};
    uint        mColumn{};
    char mRing[TR_RING_SIZE]{};
    std::streamsize mIn{};
    std::streamsize mOut{};

    TokenReaderT(std::istream &istream) noexcept : mIStream{istream} { }
    TokenReaderT(const TokenReaderT&) = default;
};


// The maximum identifier length used when processing the data set
// definition.
#define MAX_IDENT_LEN                (16)

// The limits for the listener's head 'radius' in the data set definition.
#define MIN_RADIUS                   (0.05)
#define MAX_RADIUS                   (0.15)

// The maximum number of channels that can be addressed for a WAVE file
// source listed in the data set definition.
#define MAX_WAVE_CHANNELS            (65535)

// The limits to the byte size for a binary source listed in the definition
// file.
#define MIN_BIN_SIZE                 (2)
#define MAX_BIN_SIZE                 (4)

// The minimum number of significant bits for binary sources listed in the
// data set definition.  The maximum is calculated from the byte size.
#define MIN_BIN_BITS                 (16)

// The limits to the number of significant bits for an ASCII source listed in
// the data set definition.
#define MIN_ASCII_BITS               (16)
#define MAX_ASCII_BITS               (32)

// The four-character-codes for RIFF/RIFX WAVE file chunks.
#define FOURCC_RIFF                  (0x46464952) // 'RIFF'
#define FOURCC_RIFX                  (0x58464952) // 'RIFX'
#define FOURCC_WAVE                  (0x45564157) // 'WAVE'
#define FOURCC_FMT                   (0x20746D66) // 'fmt '
#define FOURCC_DATA                  (0x61746164) // 'data'
#define FOURCC_LIST                  (0x5453494C) // 'LIST'
#define FOURCC_WAVL                  (0x6C766177) // 'wavl'
#define FOURCC_SLNT                  (0x746E6C73) // 'slnt'

// The supported wave formats.
#define WAVE_FORMAT_PCM              (0x0001)
#define WAVE_FORMAT_IEEE_FLOAT       (0x0003)
#define WAVE_FORMAT_EXTENSIBLE       (0xFFFE)


enum ByteOrderT {
    BO_NONE,
    BO_LITTLE,
    BO_BIG
};

// Source format for the references listed in the data set definition.
enum SourceFormatT {
    SF_NONE,
    SF_ASCII,  // ASCII text file.
    SF_BIN_LE, // Little-endian binary file.
    SF_BIN_BE, // Big-endian binary file.
    SF_WAVE,   // RIFF/RIFX WAVE file.
    SF_SOFA    // Spatially Oriented Format for Accoustics (SOFA) file.
};

// Element types for the references listed in the data set definition.
enum ElementTypeT {
    ET_NONE,
    ET_INT,  // Integer elements.
    ET_FP    // Floating-point elements.
};

// Source reference state used when loading sources.
struct SourceRefT {
    SourceFormatT mFormat;
    ElementTypeT  mType;
    uint mSize;
    int  mBits;
    uint mChannel;
    double mAzimuth;
    double mElevation;
    double mRadius;
    uint mSkip;
    uint mOffset;
    char mPath[MAX_PATH_LEN+1];
};


/* Whitespace is not significant. It can process tokens as identifiers, numbers
 * (integer and floating-point), strings, and operators. Strings must be
 * encapsulated by double-quotes and cannot span multiple lines.
 */

// Setup the reader on the given file.  The filename can be NULL if no error
// output is desired.
static void TrSetup(const char *startbytes, std::streamsize startbytecount, const char *filename,
    TokenReaderT *tr)
{
    const char *name = nullptr;

    if(filename)
    {
        const char *slash = strrchr(filename, '/');
        if(slash)
        {
            const char *bslash = strrchr(slash+1, '\\');
            if(bslash) name = bslash+1;
            else name = slash+1;
        }
        else
        {
            const char *bslash = strrchr(filename, '\\');
            if(bslash) name = bslash+1;
            else name = filename;
        }
    }

    tr->mName = name;
    tr->mLine = 1;
    tr->mColumn = 1;
    tr->mIn = 0;
    tr->mOut = 0;

    if(startbytecount > 0)
    {
        std::copy_n(startbytes, startbytecount, std::begin(tr->mRing));
        tr->mIn += startbytecount;
    }
}

// Prime the reader's ring buffer, and return a result indicating that there
// is text to process.
static int TrLoad(TokenReaderT *tr)
{
    std::istream &istream = tr->mIStream;

    std::streamsize toLoad{TR_RING_SIZE - static_cast<std::streamsize>(tr->mIn - tr->mOut)};
    if(toLoad >= TR_LOAD_SIZE && istream.good())
    {
        // Load TR_LOAD_SIZE (or less if at the end of the file) per read.
        toLoad = TR_LOAD_SIZE;
        std::streamsize in{tr->mIn&TR_RING_MASK};
        std::streamsize count{TR_RING_SIZE - in};
        if(count < toLoad)
        {
            istream.read(&tr->mRing[in], count);
            tr->mIn += istream.gcount();
            istream.read(&tr->mRing[0], toLoad-count);
            tr->mIn += istream.gcount();
        }
        else
        {
            istream.read(&tr->mRing[in], toLoad);
            tr->mIn += istream.gcount();
        }

        if(tr->mOut >= TR_RING_SIZE)
        {
            tr->mOut -= TR_RING_SIZE;
            tr->mIn -= TR_RING_SIZE;
        }
    }
    if(tr->mIn > tr->mOut)
        return 1;
    return 0;
}

// Error display routine.  Only displays when the base name is not NULL.
static void TrErrorVA(const TokenReaderT *tr, uint line, uint column, const char *format, va_list argPtr)
{
    if(!tr->mName)
        return;
    fprintf(stderr, "\nError (%s:%u:%u): ", tr->mName, line, column);
    vfprintf(stderr, format, argPtr);
}

// Used to display an error at a saved line/column.
static void TrErrorAt(const TokenReaderT *tr, uint line, uint column, const char *format, ...)
{
    va_list argPtr;

    va_start(argPtr, format);
    TrErrorVA(tr, line, column, format, argPtr);
    va_end(argPtr);
}

// Used to display an error at the current line/column.
static void TrError(const TokenReaderT *tr, const char *format, ...)
{
    va_list argPtr;

    va_start(argPtr, format);
    TrErrorVA(tr, tr->mLine, tr->mColumn, format, argPtr);
    va_end(argPtr);
}

// Skips to the next line.
static void TrSkipLine(TokenReaderT *tr)
{
    char ch;

    while(TrLoad(tr))
    {
        ch = tr->mRing[tr->mOut&TR_RING_MASK];
        tr->mOut++;
        if(ch == '\n')
        {
            tr->mLine++;
            tr->mColumn = 1;
            break;
        }
        tr->mColumn ++;
    }
}

// Skips to the next token.
static int TrSkipWhitespace(TokenReaderT *tr)
{
    while(TrLoad(tr))
    {
        char ch{tr->mRing[tr->mOut&TR_RING_MASK]};
        if(isspace(ch))
        {
            tr->mOut++;
            if(ch == '\n')
            {
                tr->mLine++;
                tr->mColumn = 1;
            }
            else
                tr->mColumn++;
        }
        else if(ch == '#')
            TrSkipLine(tr);
        else
            return 1;
    }
    return 0;
}

// Get the line and/or column of the next token (or the end of input).
static void TrIndication(TokenReaderT *tr, uint *line, uint *column)
{
    TrSkipWhitespace(tr);
    if(line) *line = tr->mLine;
    if(column) *column = tr->mColumn;
}

// Checks to see if a token is (likely to be) an identifier.  It does not
// display any errors and will not proceed to the next token.
static int TrIsIdent(TokenReaderT *tr)
{
    if(!TrSkipWhitespace(tr))
        return 0;
    char ch{tr->mRing[tr->mOut&TR_RING_MASK]};
    return ch == '_' || isalpha(ch);
}


// Checks to see if a token is the given operator.  It does not display any
// errors and will not proceed to the next token.
static int TrIsOperator(TokenReaderT *tr, const char *op)
{
    std::streamsize out;
    size_t len;
    char ch;

    if(!TrSkipWhitespace(tr))
        return 0;
    out = tr->mOut;
    len = 0;
    while(op[len] != '\0' && out < tr->mIn)
    {
        ch = tr->mRing[out&TR_RING_MASK];
        if(ch != op[len]) break;
        len++;
        out++;
    }
    if(op[len] == '\0')
        return 1;
    return 0;
}

/* The TrRead*() routines obtain the value of a matching token type.  They
 * display type, form, and boundary errors and will proceed to the next
 * token.
 */

// Reads and validates an identifier token.
static int TrReadIdent(TokenReaderT *tr, const uint maxLen, char *ident)
{
    uint col, len;
    char ch;

    col = tr->mColumn;
    if(TrSkipWhitespace(tr))
    {
        col = tr->mColumn;
        ch = tr->mRing[tr->mOut&TR_RING_MASK];
        if(ch == '_' || isalpha(ch))
        {
            len = 0;
            do {
                if(len < maxLen)
                    ident[len] = ch;
                len++;
                tr->mOut++;
                if(!TrLoad(tr))
                    break;
                ch = tr->mRing[tr->mOut&TR_RING_MASK];
            } while(ch == '_' || isdigit(ch) || isalpha(ch));

            tr->mColumn += len;
            if(len < maxLen)
            {
                ident[len] = '\0';
                return 1;
            }
            TrErrorAt(tr, tr->mLine, col, "Identifier is too long.\n");
            return 0;
        }
    }
    TrErrorAt(tr, tr->mLine, col, "Expected an identifier.\n");
    return 0;
}

// Reads and validates (including bounds) an integer token.
static int TrReadInt(TokenReaderT *tr, const int loBound, const int hiBound, int *value)
{
    uint col, digis, len;
    char ch, temp[64+1];

    col = tr->mColumn;
    if(TrSkipWhitespace(tr))
    {
        col = tr->mColumn;
        len = 0;
        ch = tr->mRing[tr->mOut&TR_RING_MASK];
        if(ch == '+' || ch == '-')
        {
            temp[len] = ch;
            len++;
            tr->mOut++;
        }
        digis = 0;
        while(TrLoad(tr))
        {
            ch = tr->mRing[tr->mOut&TR_RING_MASK];
            if(!isdigit(ch)) break;
            if(len < 64)
                temp[len] = ch;
            len++;
            digis++;
            tr->mOut++;
        }
        tr->mColumn += len;
        if(digis > 0 && ch != '.' && !isalpha(ch))
        {
            if(len > 64)
            {
                TrErrorAt(tr, tr->mLine, col, "Integer is too long.");
                return 0;
            }
            temp[len] = '\0';
            *value = static_cast<int>(strtol(temp, nullptr, 10));
            if(*value < loBound || *value > hiBound)
            {
                TrErrorAt(tr, tr->mLine, col, "Expected a value from %d to %d.\n", loBound, hiBound);
                return 0;
            }
            return 1;
        }
    }
    TrErrorAt(tr, tr->mLine, col, "Expected an integer.\n");
    return 0;
}

// Reads and validates (including bounds) a float token.
static int TrReadFloat(TokenReaderT *tr, const double loBound, const double hiBound, double *value)
{
    uint col, digis, len;
    char ch, temp[64+1];

    col = tr->mColumn;
    if(TrSkipWhitespace(tr))
    {
        col = tr->mColumn;
        len = 0;
        ch = tr->mRing[tr->mOut&TR_RING_MASK];
        if(ch == '+' || ch == '-')
        {
            temp[len] = ch;
            len++;
            tr->mOut++;
        }

        digis = 0;
        while(TrLoad(tr))
        {
            ch = tr->mRing[tr->mOut&TR_RING_MASK];
            if(!isdigit(ch)) break;
            if(len < 64)
                temp[len] = ch;
            len++;
            digis++;
            tr->mOut++;
        }
        if(ch == '.')
        {
            if(len < 64)
                temp[len] = ch;
            len++;
            tr->mOut++;
        }
        while(TrLoad(tr))
        {
            ch = tr->mRing[tr->mOut&TR_RING_MASK];
            if(!isdigit(ch)) break;
            if(len < 64)
                temp[len] = ch;
            len++;
            digis++;
            tr->mOut++;
        }
        if(digis > 0)
        {
            if(ch == 'E' || ch == 'e')
            {
                if(len < 64)
                    temp[len] = ch;
                len++;
                digis = 0;
                tr->mOut++;
                if(ch == '+' || ch == '-')
                {
                    if(len < 64)
                        temp[len] = ch;
                    len++;
                    tr->mOut++;
                }
                while(TrLoad(tr))
                {
                    ch = tr->mRing[tr->mOut&TR_RING_MASK];
                    if(!isdigit(ch)) break;
                    if(len < 64)
                        temp[len] = ch;
                    len++;
                    digis++;
                    tr->mOut++;
                }
            }
            tr->mColumn += len;
            if(digis > 0 && ch != '.' && !isalpha(ch))
            {
                if(len > 64)
                {
                    TrErrorAt(tr, tr->mLine, col, "Float is too long.");
                    return 0;
                }
                temp[len] = '\0';
                *value = strtod(temp, nullptr);
                if(*value < loBound || *value > hiBound)
                {
                    TrErrorAt(tr, tr->mLine, col, "Expected a value from %f to %f.\n", loBound, hiBound);
                    return 0;
                }
                return 1;
            }
        }
        else
            tr->mColumn += len;
    }
    TrErrorAt(tr, tr->mLine, col, "Expected a float.\n");
    return 0;
}

// Reads and validates a string token.
static int TrReadString(TokenReaderT *tr, const uint maxLen, char *text)
{
    uint col, len;
    char ch;

    col = tr->mColumn;
    if(TrSkipWhitespace(tr))
    {
        col = tr->mColumn;
        ch = tr->mRing[tr->mOut&TR_RING_MASK];
        if(ch == '\"')
        {
            tr->mOut++;
            len = 0;
            while(TrLoad(tr))
            {
                ch = tr->mRing[tr->mOut&TR_RING_MASK];
                tr->mOut++;
                if(ch == '\"')
                    break;
                if(ch == '\n')
                {
                    TrErrorAt(tr, tr->mLine, col, "Unterminated string at end of line.\n");
                    return 0;
                }
                if(len < maxLen)
                    text[len] = ch;
                len++;
            }
            if(ch != '\"')
            {
                tr->mColumn += 1 + len;
                TrErrorAt(tr, tr->mLine, col, "Unterminated string at end of input.\n");
                return 0;
            }
            tr->mColumn += 2 + len;
            if(len > maxLen)
            {
                TrErrorAt(tr, tr->mLine, col, "String is too long.\n");
                return 0;
            }
            text[len] = '\0';
            return 1;
        }
    }
    TrErrorAt(tr, tr->mLine, col, "Expected a string.\n");
    return 0;
}

// Reads and validates the given operator.
static int TrReadOperator(TokenReaderT *tr, const char *op)
{
    uint col, len;
    char ch;

    col = tr->mColumn;
    if(TrSkipWhitespace(tr))
    {
        col = tr->mColumn;
        len = 0;
        while(op[len] != '\0' && TrLoad(tr))
        {
            ch = tr->mRing[tr->mOut&TR_RING_MASK];
            if(ch != op[len]) break;
            len++;
            tr->mOut++;
        }
        tr->mColumn += len;
        if(op[len] == '\0')
            return 1;
    }
    TrErrorAt(tr, tr->mLine, col, "Expected '%s' operator.\n", op);
    return 0;
}


/*************************
 *** File source input ***
 *************************/

// Read a binary value of the specified byte order and byte size from a file,
// storing it as a 32-bit unsigned integer.
static int ReadBin4(std::istream &istream, const char *filename, const ByteOrderT order, const uint bytes, uint32_t *out)
{
    uint8_t in[4];
    istream.read(reinterpret_cast<char*>(in), static_cast<int>(bytes));
    if(istream.gcount() != bytes)
    {
        fprintf(stderr, "\nError: Bad read from file '%s'.\n", filename);
        return 0;
    }
    uint32_t accum{0};
    switch(order)
    {
        case BO_LITTLE:
            for(uint i = 0;i < bytes;i++)
                accum = (accum<<8) | in[bytes - i - 1];
            break;
        case BO_BIG:
            for(uint i = 0;i < bytes;i++)
                accum = (accum<<8) | in[i];
            break;
        default:
            break;
    }
    *out = accum;
    return 1;
}

// Read a binary value of the specified byte order from a file, storing it as
// a 64-bit unsigned integer.
static int ReadBin8(std::istream &istream, const char *filename, const ByteOrderT order, uint64_t *out)
{
    uint8_t in[8];
    uint64_t accum;
    uint i;

    istream.read(reinterpret_cast<char*>(in), 8);
    if(istream.gcount() != 8)
    {
        fprintf(stderr, "\nError: Bad read from file '%s'.\n", filename);
        return 0;
    }
    accum = 0;
    switch(order)
    {
        case BO_LITTLE:
            for(i = 0;i < 8;i++)
                accum = (accum<<8) | in[8 - i - 1];
            break;
        case BO_BIG:
            for(i = 0;i < 8;i++)
                accum = (accum<<8) | in[i];
            break;
        default:
            break;
    }
    *out = accum;
    return 1;
}

/* Read a binary value of the specified type, byte order, and byte size from
 * a file, converting it to a double.  For integer types, the significant
 * bits are used to normalize the result.  The sign of bits determines
 * whether they are padded toward the MSB (negative) or LSB (positive).
 * Floating-point types are not normalized.
 */
static int ReadBinAsDouble(std::istream &istream, const char *filename, const ByteOrderT order,
    const ElementTypeT type, const uint bytes, const int bits, double *out)
{
    union {
        uint32_t ui;
        int32_t i;
        float f;
    } v4;
    union {
        uint64_t ui;
        double f;
    } v8;

    *out = 0.0;
    if(bytes > 4)
    {
        if(!ReadBin8(istream, filename, order, &v8.ui))
            return 0;
        if(type == ET_FP)
            *out = v8.f;
    }
    else
    {
        if(!ReadBin4(istream, filename, order, bytes, &v4.ui))
            return 0;
        if(type == ET_FP)
            *out = v4.f;
        else
        {
            if(bits > 0)
                v4.ui >>= (8*bytes) - (static_cast<uint>(bits));
            else
                v4.ui &= (0xFFFFFFFF >> (32+bits));

            if(v4.ui&static_cast<uint>(1<<(std::abs(bits)-1)))
                v4.ui |= (0xFFFFFFFF << std::abs(bits));
            *out = v4.i / static_cast<double>(1<<(std::abs(bits)-1));
        }
    }
    return 1;
}

/* Read an ascii value of the specified type from a file, converting it to a
 * double.  For integer types, the significant bits are used to normalize the
 * result.  The sign of the bits should always be positive.  This also skips
 * up to one separator character before the element itself.
 */
static int ReadAsciiAsDouble(TokenReaderT *tr, const char *filename, const ElementTypeT type, const uint bits, double *out)
{
    if(TrIsOperator(tr, ","))
        TrReadOperator(tr, ",");
    else if(TrIsOperator(tr, ":"))
        TrReadOperator(tr, ":");
    else if(TrIsOperator(tr, ";"))
        TrReadOperator(tr, ";");
    else if(TrIsOperator(tr, "|"))
        TrReadOperator(tr, "|");

    if(type == ET_FP)
    {
        if(!TrReadFloat(tr, -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity(), out))
        {
            fprintf(stderr, "\nError: Bad read from file '%s'.\n", filename);
            return 0;
        }
    }
    else
    {
        int v;
        if(!TrReadInt(tr, -(1<<(bits-1)), (1<<(bits-1))-1, &v))
        {
            fprintf(stderr, "\nError: Bad read from file '%s'.\n", filename);
            return 0;
        }
        *out = v / static_cast<double>((1<<(bits-1))-1);
    }
    return 1;
}

// Read the RIFF/RIFX WAVE format chunk from a file, validating it against
// the source parameters and data set metrics.
static int ReadWaveFormat(std::istream &istream, const ByteOrderT order, const uint hrirRate,
    SourceRefT *src)
{
    uint32_t fourCC, chunkSize;
    uint32_t format, channels, rate, dummy, block, size, bits;

    chunkSize = 0;
    do {
        if(chunkSize > 0)
            istream.seekg(static_cast<int>(chunkSize), std::ios::cur);
        if(!ReadBin4(istream, src->mPath, BO_LITTLE, 4, &fourCC)
            || !ReadBin4(istream, src->mPath, order, 4, &chunkSize))
            return 0;
    } while(fourCC != FOURCC_FMT);
    if(!ReadBin4(istream, src->mPath, order, 2, &format)
        || !ReadBin4(istream, src->mPath, order, 2, &channels)
        || !ReadBin4(istream, src->mPath, order, 4, &rate)
        || !ReadBin4(istream, src->mPath, order, 4, &dummy)
        || !ReadBin4(istream, src->mPath, order, 2, &block))
        return 0;
    block /= channels;
    if(chunkSize > 14)
    {
        if(!ReadBin4(istream, src->mPath, order, 2, &size))
            return 0;
        size /= 8;
        if(block > size)
            size = block;
    }
    else
        size = block;
    if(format == WAVE_FORMAT_EXTENSIBLE)
    {
        istream.seekg(2, std::ios::cur);
        if(!ReadBin4(istream, src->mPath, order, 2, &bits))
            return 0;
        if(bits == 0)
            bits = 8 * size;
        istream.seekg(4, std::ios::cur);
        if(!ReadBin4(istream, src->mPath, order, 2, &format))
            return 0;
        istream.seekg(static_cast<int>(chunkSize - 26), std::ios::cur);
    }
    else
    {
        bits = 8 * size;
        if(chunkSize > 14)
            istream.seekg(static_cast<int>(chunkSize - 16), std::ios::cur);
        else
            istream.seekg(static_cast<int>(chunkSize - 14), std::ios::cur);
    }
    if(format != WAVE_FORMAT_PCM && format != WAVE_FORMAT_IEEE_FLOAT)
    {
        fprintf(stderr, "\nError: Unsupported WAVE format in file '%s'.\n", src->mPath);
        return 0;
    }
    if(src->mChannel >= channels)
    {
        fprintf(stderr, "\nError: Missing source channel in WAVE file '%s'.\n", src->mPath);
        return 0;
    }
    if(rate != hrirRate)
    {
        fprintf(stderr, "\nError: Mismatched source sample rate in WAVE file '%s'.\n", src->mPath);
        return 0;
    }
    if(format == WAVE_FORMAT_PCM)
    {
        if(size < 2 || size > 4)
        {
            fprintf(stderr, "\nError: Unsupported sample size in WAVE file '%s'.\n", src->mPath);
            return 0;
        }
        if(bits < 16 || bits > (8*size))
        {
            fprintf(stderr, "\nError: Bad significant bits in WAVE file '%s'.\n", src->mPath);
            return 0;
        }
        src->mType = ET_INT;
    }
    else
    {
        if(size != 4 && size != 8)
        {
            fprintf(stderr, "\nError: Unsupported sample size in WAVE file '%s'.\n", src->mPath);
            return 0;
        }
        src->mType = ET_FP;
    }
    src->mSize = size;
    src->mBits = static_cast<int>(bits);
    src->mSkip = channels;
    return 1;
}

// Read a RIFF/RIFX WAVE data chunk, converting all elements to doubles.
static int ReadWaveData(std::istream &istream, const SourceRefT *src, const ByteOrderT order,
    const uint n, double *hrir)
{
    int pre, post, skip;
    uint i;

    pre = static_cast<int>(src->mSize * src->mChannel);
    post = static_cast<int>(src->mSize * (src->mSkip - src->mChannel - 1));
    skip = 0;
    for(i = 0;i < n;i++)
    {
        skip += pre;
        if(skip > 0)
            istream.seekg(skip, std::ios::cur);
        if(!ReadBinAsDouble(istream, src->mPath, order, src->mType, src->mSize, src->mBits, &hrir[i]))
            return 0;
        skip = post;
    }
    if(skip > 0)
        istream.seekg(skip, std::ios::cur);
    return 1;
}

// Read the RIFF/RIFX WAVE list or data chunk, converting all elements to
// doubles.
static int ReadWaveList(std::istream &istream, const SourceRefT *src, const ByteOrderT order,
    const uint n, double *hrir)
{
    uint32_t fourCC, chunkSize, listSize, count;
    uint block, skip, offset, i;
    double lastSample;

    for(;;)
    {
        if(!ReadBin4(istream, src->mPath, BO_LITTLE, 4, &fourCC)
            || !ReadBin4(istream, src->mPath, order, 4, &chunkSize))
            return 0;

        if(fourCC == FOURCC_DATA)
        {
            block = src->mSize * src->mSkip;
            count = chunkSize / block;
            if(count < (src->mOffset + n))
            {
                fprintf(stderr, "\nError: Bad read from file '%s'.\n", src->mPath);
                return 0;
            }
            istream.seekg(static_cast<long>(src->mOffset * block), std::ios::cur);
            if(!ReadWaveData(istream, src, order, n, &hrir[0]))
                return 0;
            return 1;
        }
        else if(fourCC == FOURCC_LIST)
        {
            if(!ReadBin4(istream, src->mPath, BO_LITTLE, 4, &fourCC))
                return 0;
            chunkSize -= 4;
            if(fourCC == FOURCC_WAVL)
                break;
        }
        if(chunkSize > 0)
            istream.seekg(static_cast<long>(chunkSize), std::ios::cur);
    }
    listSize = chunkSize;
    block = src->mSize * src->mSkip;
    skip = src->mOffset;
    offset = 0;
    lastSample = 0.0;
    while(offset < n && listSize > 8)
    {
        if(!ReadBin4(istream, src->mPath, BO_LITTLE, 4, &fourCC)
            || !ReadBin4(istream, src->mPath, order, 4, &chunkSize))
            return 0;
        listSize -= 8 + chunkSize;
        if(fourCC == FOURCC_DATA)
        {
            count = chunkSize / block;
            if(count > skip)
            {
                istream.seekg(static_cast<long>(skip * block), std::ios::cur);
                chunkSize -= skip * block;
                count -= skip;
                skip = 0;
                if(count > (n - offset))
                    count = n - offset;
                if(!ReadWaveData(istream, src, order, count, &hrir[offset]))
                    return 0;
                chunkSize -= count * block;
                offset += count;
                lastSample = hrir[offset - 1];
            }
            else
            {
                skip -= count;
                count = 0;
            }
        }
        else if(fourCC == FOURCC_SLNT)
        {
            if(!ReadBin4(istream, src->mPath, order, 4, &count))
                return 0;
            chunkSize -= 4;
            if(count > skip)
            {
                count -= skip;
                skip = 0;
                if(count > (n - offset))
                    count = n - offset;
                for(i = 0; i < count; i ++)
                    hrir[offset + i] = lastSample;
                offset += count;
            }
            else
            {
                skip -= count;
                count = 0;
            }
        }
        if(chunkSize > 0)
            istream.seekg(static_cast<long>(chunkSize), std::ios::cur);
    }
    if(offset < n)
    {
        fprintf(stderr, "\nError: Bad read from file '%s'.\n", src->mPath);
        return 0;
    }
    return 1;
}

// Load a source HRIR from an ASCII text file containing a list of elements
// separated by whitespace or common list operators (',', ';', ':', '|').
static int LoadAsciiSource(std::istream &istream, const SourceRefT *src,
    const uint n, double *hrir)
{
    TokenReaderT tr{istream};
    uint i, j;
    double dummy;

    TrSetup(nullptr, 0, nullptr, &tr);
    for(i = 0;i < src->mOffset;i++)
    {
        if(!ReadAsciiAsDouble(&tr, src->mPath, src->mType, static_cast<uint>(src->mBits), &dummy))
            return 0;
    }
    for(i = 0;i < n;i++)
    {
        if(!ReadAsciiAsDouble(&tr, src->mPath, src->mType, static_cast<uint>(src->mBits), &hrir[i]))
            return 0;
        for(j = 0;j < src->mSkip;j++)
        {
            if(!ReadAsciiAsDouble(&tr, src->mPath, src->mType, static_cast<uint>(src->mBits), &dummy))
                return 0;
        }
    }
    return 1;
}

// Load a source HRIR from a binary file.
static int LoadBinarySource(std::istream &istream, const SourceRefT *src, const ByteOrderT order,
    const uint n, double *hrir)
{
    istream.seekg(static_cast<long>(src->mOffset), std::ios::beg);
    for(uint i{0};i < n;i++)
    {
        if(!ReadBinAsDouble(istream, src->mPath, order, src->mType, src->mSize, src->mBits, &hrir[i]))
            return 0;
        if(src->mSkip > 0)
            istream.seekg(static_cast<long>(src->mSkip), std::ios::cur);
    }
    return 1;
}

// Load a source HRIR from a RIFF/RIFX WAVE file.
static int LoadWaveSource(std::istream &istream, SourceRefT *src, const uint hrirRate,
    const uint n, double *hrir)
{
    uint32_t fourCC, dummy;
    ByteOrderT order;

    if(!ReadBin4(istream, src->mPath, BO_LITTLE, 4, &fourCC)
        || !ReadBin4(istream, src->mPath, BO_LITTLE, 4, &dummy))
        return 0;
    if(fourCC == FOURCC_RIFF)
        order = BO_LITTLE;
    else if(fourCC == FOURCC_RIFX)
        order = BO_BIG;
    else
    {
        fprintf(stderr, "\nError: No RIFF/RIFX chunk in file '%s'.\n", src->mPath);
        return 0;
    }

    if(!ReadBin4(istream, src->mPath, BO_LITTLE, 4, &fourCC))
        return 0;
    if(fourCC != FOURCC_WAVE)
    {
        fprintf(stderr, "\nError: Not a RIFF/RIFX WAVE file '%s'.\n", src->mPath);
        return 0;
    }
    if(!ReadWaveFormat(istream, order, hrirRate, src))
        return 0;
    if(!ReadWaveList(istream, src, order, n, hrir))
        return 0;
    return 1;
}



// Load a Spatially Oriented Format for Accoustics (SOFA) file.
static MYSOFA_EASY* LoadSofaFile(SourceRefT *src, const uint hrirRate, const uint n)
{
    struct MYSOFA_EASY *sofa{mysofa_cache_lookup(src->mPath, static_cast<float>(hrirRate))};
    if(sofa) return sofa;

    sofa = static_cast<MYSOFA_EASY*>(calloc(1, sizeof(*sofa)));
    if(sofa == nullptr)
    {
        fprintf(stderr, "\nError:  Out of memory.\n");
        return nullptr;
    }
    sofa->lookup = nullptr;
    sofa->neighborhood = nullptr;

    int err;
    sofa->hrtf = mysofa_load(src->mPath, &err);
    if(!sofa->hrtf)
    {
        mysofa_close(sofa);
        fprintf(stderr, "\nError: Could not load source file '%s'.\n", src->mPath);
        return nullptr;
    }
    /* NOTE: Some valid SOFA files are failing this check. */
    err = mysofa_check(sofa->hrtf);
    if(err != MYSOFA_OK)
        fprintf(stderr, "\nWarning: Supposedly malformed source file '%s'.\n", src->mPath);
    if((src->mOffset + n) > sofa->hrtf->N)
    {
        mysofa_close(sofa);
        fprintf(stderr, "\nError: Not enough samples in SOFA file '%s'.\n", src->mPath);
        return nullptr;
    }
    if(src->mChannel >= sofa->hrtf->R)
    {
        mysofa_close(sofa);
        fprintf(stderr, "\nError: Missing source receiver in SOFA file '%s'.\n", src->mPath);
        return nullptr;
    }
    mysofa_tocartesian(sofa->hrtf);
    sofa->lookup = mysofa_lookup_init(sofa->hrtf);
    if(sofa->lookup == nullptr)
    {
        mysofa_close(sofa);
        fprintf(stderr, "\nError:  Out of memory.\n");
        return nullptr;
    }
    return mysofa_cache_store(sofa, src->mPath, static_cast<float>(hrirRate));
}

// Copies the HRIR data from a particular SOFA measurement.
static void ExtractSofaHrir(const MYSOFA_EASY *sofa, const uint index, const uint channel, const uint offset, const uint n, double *hrir)
{
    for(uint i{0u};i < n;i++)
        hrir[i] = sofa->hrtf->DataIR.values[(index*sofa->hrtf->R + channel)*sofa->hrtf->N + offset + i];
}

// Load a source HRIR from a Spatially Oriented Format for Accoustics (SOFA)
// file.
static int LoadSofaSource(SourceRefT *src, const uint hrirRate, const uint n, double *hrir)
{
    struct MYSOFA_EASY *sofa;
    float target[3];
    int nearest;
    float *coords;

    sofa = LoadSofaFile(src, hrirRate, n);
    if(sofa == nullptr)
        return 0;

    /* NOTE: At some point it may be benficial or necessary to consider the
             various coordinate systems, listener/source orientations, and
             direciontal vectors defined in the SOFA file.
    */
    target[0] = static_cast<float>(src->mAzimuth);
    target[1] = static_cast<float>(src->mElevation);
    target[2] = static_cast<float>(src->mRadius);
    mysofa_s2c(target);

    nearest = mysofa_lookup(sofa->lookup, target);
    if(nearest < 0)
    {
        fprintf(stderr, "\nError: Lookup failed in source file '%s'.\n", src->mPath);
        return 0;
    }

    coords = &sofa->hrtf->SourcePosition.values[3 * nearest];
    if(std::abs(coords[0] - target[0]) > 0.001 || std::abs(coords[1] - target[1]) > 0.001 || std::abs(coords[2] - target[2]) > 0.001)
    {
        fprintf(stderr, "\nError: No impulse response at coordinates (%.3fr, %.1fev, %.1faz) in file '%s'.\n", src->mRadius, src->mElevation, src->mAzimuth, src->mPath);
        target[0] = coords[0];
        target[1] = coords[1];
        target[2] = coords[2];
        mysofa_c2s(target);
        fprintf(stderr, "       Nearest candidate at (%.3fr, %.1fev, %.1faz).\n", target[2], target[1], target[0]);
        return 0;
    }

    ExtractSofaHrir(sofa, static_cast<uint>(nearest), src->mChannel, src->mOffset, n, hrir);

    return 1;
}

// Load a source HRIR from a supported file type.
static int LoadSource(SourceRefT *src, const uint hrirRate, const uint n, double *hrir)
{
    std::unique_ptr<al::ifstream> istream;
    if(src->mFormat != SF_SOFA)
    {
        if(src->mFormat == SF_ASCII)
            istream.reset(new al::ifstream{src->mPath});
        else
            istream.reset(new al::ifstream{src->mPath, std::ios::binary});
        if(!istream->good())
        {
            fprintf(stderr, "\nError: Could not open source file '%s'.\n", src->mPath);
            return 0;
        }
    }
    int result{0};
    switch(src->mFormat)
    {
        case SF_ASCII:
            result = LoadAsciiSource(*istream, src, n, hrir);
            break;
        case SF_BIN_LE:
            result = LoadBinarySource(*istream, src, BO_LITTLE, n, hrir);
            break;
        case SF_BIN_BE:
            result = LoadBinarySource(*istream, src, BO_BIG, n, hrir);
            break;
        case SF_WAVE:
            result = LoadWaveSource(*istream, src, hrirRate, n, hrir);
            break;
        case SF_SOFA:
            result = LoadSofaSource(src, hrirRate, n, hrir);
            break;
        case SF_NONE:
            break;
    }
    return result;
}


// Match the channel type from a given identifier.
static ChannelTypeT MatchChannelType(const char *ident)
{
    if(al::strcasecmp(ident, "mono") == 0)
        return CT_MONO;
    if(al::strcasecmp(ident, "stereo") == 0)
        return CT_STEREO;
    return CT_NONE;
}


// Process the data set definition to read and validate the data set metrics.
static int ProcessMetrics(TokenReaderT *tr, const uint fftSize, const uint truncSize, const ChannelModeT chanMode, HrirDataT *hData)
{
    int hasRate = 0, hasType = 0, hasPoints = 0, hasRadius = 0;
    int hasDistance = 0, hasAzimuths = 0;
    char ident[MAX_IDENT_LEN+1];
    uint line, col;
    double fpVal;
    uint points;
    int intVal;
    double distances[MAX_FD_COUNT];
    uint fdCount = 0;
    uint evCounts[MAX_FD_COUNT];
    std::vector<uint> azCounts(MAX_FD_COUNT * MAX_EV_COUNT);

    TrIndication(tr, &line, &col);
    while(TrIsIdent(tr))
    {
        TrIndication(tr, &line, &col);
        if(!TrReadIdent(tr, MAX_IDENT_LEN, ident))
            return 0;
        if(al::strcasecmp(ident, "rate") == 0)
        {
            if(hasRate)
            {
                TrErrorAt(tr, line, col, "Redefinition of 'rate'.\n");
                return 0;
            }
            if(!TrReadOperator(tr, "="))
                return 0;
            if(!TrReadInt(tr, MIN_RATE, MAX_RATE, &intVal))
                return 0;
            hData->mIrRate = static_cast<uint>(intVal);
            hasRate = 1;
        }
        else if(al::strcasecmp(ident, "type") == 0)
        {
            char type[MAX_IDENT_LEN+1];

            if(hasType)
            {
                TrErrorAt(tr, line, col, "Redefinition of 'type'.\n");
                return 0;
            }
            if(!TrReadOperator(tr, "="))
                return 0;

            if(!TrReadIdent(tr, MAX_IDENT_LEN, type))
                return 0;
            hData->mChannelType = MatchChannelType(type);
            if(hData->mChannelType == CT_NONE)
            {
                TrErrorAt(tr, line, col, "Expected a channel type.\n");
                return 0;
            }
            else if(hData->mChannelType == CT_STEREO)
            {
                if(chanMode == CM_ForceMono)
                    hData->mChannelType = CT_MONO;
            }
            hasType = 1;
        }
        else if(al::strcasecmp(ident, "points") == 0)
        {
            if(hasPoints)
            {
                TrErrorAt(tr, line, col, "Redefinition of 'points'.\n");
                return 0;
            }
            if(!TrReadOperator(tr, "="))
                return 0;
            TrIndication(tr, &line, &col);
            if(!TrReadInt(tr, MIN_POINTS, MAX_POINTS, &intVal))
                return 0;
            points = static_cast<uint>(intVal);
            if(fftSize > 0 && points > fftSize)
            {
                TrErrorAt(tr, line, col, "Value exceeds the overridden FFT size.\n");
                return 0;
            }
            if(points < truncSize)
            {
                TrErrorAt(tr, line, col, "Value is below the truncation size.\n");
                return 0;
            }
            hData->mIrPoints = points;
            hData->mFftSize = fftSize;
            hData->mIrSize = 1 + (fftSize / 2);
            if(points > hData->mIrSize)
                hData->mIrSize = points;
            hasPoints = 1;
        }
        else if(al::strcasecmp(ident, "radius") == 0)
        {
            if(hasRadius)
            {
                TrErrorAt(tr, line, col, "Redefinition of 'radius'.\n");
                return 0;
            }
            if(!TrReadOperator(tr, "="))
                return 0;
            if(!TrReadFloat(tr, MIN_RADIUS, MAX_RADIUS, &fpVal))
                return 0;
            hData->mRadius = fpVal;
            hasRadius = 1;
        }
        else if(al::strcasecmp(ident, "distance") == 0)
        {
            uint count = 0;

            if(hasDistance)
            {
                TrErrorAt(tr, line, col, "Redefinition of 'distance'.\n");
                return 0;
            }
            if(!TrReadOperator(tr, "="))
                return 0;

            for(;;)
            {
                if(!TrReadFloat(tr, MIN_DISTANCE, MAX_DISTANCE, &fpVal))
                    return 0;
                if(count > 0 && fpVal <= distances[count - 1])
                {
                    TrError(tr, "Distances are not ascending.\n");
                    return 0;
                }
                distances[count++] = fpVal;
                if(!TrIsOperator(tr, ","))
                    break;
                if(count >= MAX_FD_COUNT)
                {
                    TrError(tr, "Exceeded the maximum of %d fields.\n", MAX_FD_COUNT);
                    return 0;
                }
                TrReadOperator(tr, ",");
            }
            if(fdCount != 0 && count != fdCount)
            {
                TrError(tr, "Did not match the specified number of %d fields.\n", fdCount);
                return 0;
            }
            fdCount = count;
            hasDistance = 1;
        }
        else if(al::strcasecmp(ident, "azimuths") == 0)
        {
            uint count = 0;

            if(hasAzimuths)
            {
                TrErrorAt(tr, line, col, "Redefinition of 'azimuths'.\n");
                return 0;
            }
            if(!TrReadOperator(tr, "="))
                return 0;

            evCounts[0] = 0;
            for(;;)
            {
                if(!TrReadInt(tr, MIN_AZ_COUNT, MAX_AZ_COUNT, &intVal))
                    return 0;
                azCounts[(count * MAX_EV_COUNT) + evCounts[count]++] = static_cast<uint>(intVal);
                if(TrIsOperator(tr, ","))
                {
                    if(evCounts[count] >= MAX_EV_COUNT)
                    {
                        TrError(tr, "Exceeded the maximum of %d elevations.\n", MAX_EV_COUNT);
                        return 0;
                    }
                    TrReadOperator(tr, ",");
                }
                else
                {
                    if(evCounts[count] < MIN_EV_COUNT)
                    {
                        TrErrorAt(tr, line, col, "Did not reach the minimum of %d azimuth counts.\n", MIN_EV_COUNT);
                        return 0;
                    }
                    if(azCounts[count * MAX_EV_COUNT] != 1 || azCounts[(count * MAX_EV_COUNT) + evCounts[count] - 1] != 1)
                    {
                        TrError(tr, "Poles are not singular for field %d.\n", count - 1);
                        return 0;
                    }
                    count++;
                    if(!TrIsOperator(tr, ";"))
                        break;

                    if(count >= MAX_FD_COUNT)
                    {
                        TrError(tr, "Exceeded the maximum number of %d fields.\n", MAX_FD_COUNT);
                        return 0;
                    }
                    evCounts[count] = 0;
                    TrReadOperator(tr, ";");
                }
            }
            if(fdCount != 0 && count != fdCount)
            {
                TrError(tr, "Did not match the specified number of %d fields.\n", fdCount);
                return 0;
            }
            fdCount = count;
            hasAzimuths = 1;
        }
        else
        {
            TrErrorAt(tr, line, col, "Expected a metric name.\n");
            return 0;
        }
        TrSkipWhitespace(tr);
    }
    if(!(hasRate && hasPoints && hasRadius && hasDistance && hasAzimuths))
    {
        TrErrorAt(tr, line, col, "Expected a metric name.\n");
        return 0;
    }
    if(distances[0] < hData->mRadius)
    {
        TrError(tr, "Distance cannot start below head radius.\n");
        return 0;
    }
    if(hData->mChannelType == CT_NONE)
        hData->mChannelType = CT_MONO;
    if(!PrepareHrirData(fdCount, distances, evCounts, azCounts.data(), hData))
    {
        fprintf(stderr, "Error:  Out of memory.\n");
        exit(-1);
    }
    return 1;
}

// Parse an index triplet from the data set definition.
static int ReadIndexTriplet(TokenReaderT *tr, const HrirDataT *hData, uint *fi, uint *ei, uint *ai)
{
    int intVal;

    if(hData->mFdCount > 1)
    {
        if(!TrReadInt(tr, 0, static_cast<int>(hData->mFdCount) - 1, &intVal))
            return 0;
        *fi = static_cast<uint>(intVal);
        if(!TrReadOperator(tr, ","))
            return 0;
    }
    else
    {
        *fi = 0;
    }
    if(!TrReadInt(tr, 0, static_cast<int>(hData->mFds[*fi].mEvCount) - 1, &intVal))
        return 0;
    *ei = static_cast<uint>(intVal);
    if(!TrReadOperator(tr, ","))
        return 0;
    if(!TrReadInt(tr, 0, static_cast<int>(hData->mFds[*fi].mEvs[*ei].mAzCount) - 1, &intVal))
        return 0;
    *ai = static_cast<uint>(intVal);
    return 1;
}

// Match the source format from a given identifier.
static SourceFormatT MatchSourceFormat(const char *ident)
{
    if(al::strcasecmp(ident, "ascii") == 0)
        return SF_ASCII;
    if(al::strcasecmp(ident, "bin_le") == 0)
        return SF_BIN_LE;
    if(al::strcasecmp(ident, "bin_be") == 0)
        return SF_BIN_BE;
    if(al::strcasecmp(ident, "wave") == 0)
        return SF_WAVE;
    if(al::strcasecmp(ident, "sofa") == 0)
        return SF_SOFA;
    return SF_NONE;
}

// Match the source element type from a given identifier.
static ElementTypeT MatchElementType(const char *ident)
{
    if(al::strcasecmp(ident, "int") == 0)
        return ET_INT;
    if(al::strcasecmp(ident, "fp") == 0)
        return ET_FP;
    return ET_NONE;
}

// Parse and validate a source reference from the data set definition.
static int ReadSourceRef(TokenReaderT *tr, SourceRefT *src)
{
    char ident[MAX_IDENT_LEN+1];
    uint line, col;
    double fpVal;
    int intVal;

    TrIndication(tr, &line, &col);
    if(!TrReadIdent(tr, MAX_IDENT_LEN, ident))
        return 0;
    src->mFormat = MatchSourceFormat(ident);
    if(src->mFormat == SF_NONE)
    {
        TrErrorAt(tr, line, col, "Expected a source format.\n");
        return 0;
    }
    if(!TrReadOperator(tr, "("))
        return 0;
    if(src->mFormat == SF_SOFA)
    {
        if(!TrReadFloat(tr, MIN_DISTANCE, MAX_DISTANCE, &fpVal))
            return 0;
        src->mRadius = fpVal;
        if(!TrReadOperator(tr, ","))
            return 0;
        if(!TrReadFloat(tr, -90.0, 90.0, &fpVal))
            return 0;
        src->mElevation = fpVal;
        if(!TrReadOperator(tr, ","))
            return 0;
        if(!TrReadFloat(tr, -360.0, 360.0, &fpVal))
            return 0;
        src->mAzimuth = fpVal;
        if(!TrReadOperator(tr, ":"))
            return 0;
        if(!TrReadInt(tr, 0, MAX_WAVE_CHANNELS, &intVal))
            return 0;
        src->mType = ET_NONE;
        src->mSize = 0;
        src->mBits = 0;
        src->mChannel = static_cast<uint>(intVal);
        src->mSkip = 0;
    }
    else if(src->mFormat == SF_WAVE)
    {
        if(!TrReadInt(tr, 0, MAX_WAVE_CHANNELS, &intVal))
            return 0;
        src->mType = ET_NONE;
        src->mSize = 0;
        src->mBits = 0;
        src->mChannel = static_cast<uint>(intVal);
        src->mSkip = 0;
    }
    else
    {
        TrIndication(tr, &line, &col);
        if(!TrReadIdent(tr, MAX_IDENT_LEN, ident))
            return 0;
        src->mType = MatchElementType(ident);
        if(src->mType == ET_NONE)
        {
            TrErrorAt(tr, line, col, "Expected a source element type.\n");
            return 0;
        }
        if(src->mFormat == SF_BIN_LE || src->mFormat == SF_BIN_BE)
        {
            if(!TrReadOperator(tr, ","))
                return 0;
            if(src->mType == ET_INT)
            {
                if(!TrReadInt(tr, MIN_BIN_SIZE, MAX_BIN_SIZE, &intVal))
                    return 0;
                src->mSize = static_cast<uint>(intVal);
                if(!TrIsOperator(tr, ","))
                    src->mBits = static_cast<int>(8*src->mSize);
                else
                {
                    TrReadOperator(tr, ",");
                    TrIndication(tr, &line, &col);
                    if(!TrReadInt(tr, -2147483647-1, 2147483647, &intVal))
                        return 0;
                    if(std::abs(intVal) < MIN_BIN_BITS || static_cast<uint>(std::abs(intVal)) > (8*src->mSize))
                    {
                        TrErrorAt(tr, line, col, "Expected a value of (+/-) %d to %d.\n", MIN_BIN_BITS, 8*src->mSize);
                        return 0;
                    }
                    src->mBits = intVal;
                }
            }
            else
            {
                TrIndication(tr, &line, &col);
                if(!TrReadInt(tr, -2147483647-1, 2147483647, &intVal))
                    return 0;
                if(intVal != 4 && intVal != 8)
                {
                    TrErrorAt(tr, line, col, "Expected a value of 4 or 8.\n");
                    return 0;
                }
                src->mSize = static_cast<uint>(intVal);
                src->mBits = 0;
            }
        }
        else if(src->mFormat == SF_ASCII && src->mType == ET_INT)
        {
            if(!TrReadOperator(tr, ","))
                return 0;
            if(!TrReadInt(tr, MIN_ASCII_BITS, MAX_ASCII_BITS, &intVal))
                return 0;
            src->mSize = 0;
            src->mBits = intVal;
        }
        else
        {
            src->mSize = 0;
            src->mBits = 0;
        }

        if(!TrIsOperator(tr, ";"))
            src->mSkip = 0;
        else
        {
            TrReadOperator(tr, ";");
            if(!TrReadInt(tr, 0, 0x7FFFFFFF, &intVal))
                return 0;
            src->mSkip = static_cast<uint>(intVal);
        }
    }
    if(!TrReadOperator(tr, ")"))
        return 0;
    if(TrIsOperator(tr, "@"))
    {
        TrReadOperator(tr, "@");
        if(!TrReadInt(tr, 0, 0x7FFFFFFF, &intVal))
            return 0;
        src->mOffset = static_cast<uint>(intVal);
    }
    else
        src->mOffset = 0;
    if(!TrReadOperator(tr, ":"))
        return 0;
    if(!TrReadString(tr, MAX_PATH_LEN, src->mPath))
        return 0;
    return 1;
}

// Parse and validate a SOFA source reference from the data set definition.
static int ReadSofaRef(TokenReaderT *tr, SourceRefT *src)
{
    char ident[MAX_IDENT_LEN+1];
    uint line, col;
    int intVal;

    TrIndication(tr, &line, &col);
    if(!TrReadIdent(tr, MAX_IDENT_LEN, ident))
        return 0;
    src->mFormat = MatchSourceFormat(ident);
    if(src->mFormat != SF_SOFA)
    {
        TrErrorAt(tr, line, col, "Expected the SOFA source format.\n");
        return 0;
    }

    src->mType = ET_NONE;
    src->mSize = 0;
    src->mBits = 0;
    src->mChannel = 0;
    src->mSkip = 0;

    if(TrIsOperator(tr, "@"))
    {
        TrReadOperator(tr, "@");
        if(!TrReadInt(tr, 0, 0x7FFFFFFF, &intVal))
            return 0;
        src->mOffset = static_cast<uint>(intVal);
    }
    else
        src->mOffset = 0;
    if(!TrReadOperator(tr, ":"))
        return 0;
    if(!TrReadString(tr, MAX_PATH_LEN, src->mPath))
        return 0;
    return 1;
}

// Match the target ear (index) from a given identifier.
static int MatchTargetEar(const char *ident)
{
    if(al::strcasecmp(ident, "left") == 0)
        return 0;
    if(al::strcasecmp(ident, "right") == 0)
        return 1;
    return -1;
}

// Calculate the onset time of an HRIR and average it with any existing
// timing for its field, elevation, azimuth, and ear.
static double AverageHrirOnset(const uint rate, const uint n, const double *hrir, const double f, const double onset)
{
    std::vector<double> upsampled(10 * n);
    {
        PPhaseResampler rs;
        rs.init(rate, 10 * rate);
        rs.process(n, hrir, 10 * n, upsampled.data());
    }

    auto abs_lt = [](const double &lhs, const double &rhs) -> bool
    { return std::abs(lhs) < std::abs(rhs); };
    auto iter = std::max_element(upsampled.cbegin(), upsampled.cend(), abs_lt);
    return Lerp(onset, static_cast<double>(std::distance(upsampled.cbegin(), iter))/(10*rate), f);
}

// Calculate the magnitude response of an HRIR and average it with any
// existing responses for its field, elevation, azimuth, and ear.
static void AverageHrirMagnitude(const uint points, const uint n, const double *hrir, const double f, double *mag)
{
    uint m = 1 + (n / 2), i;
    std::vector<complex_d> h(n);
    std::vector<double> r(n);

    for(i = 0;i < points;i++)
        h[i] = complex_d{hrir[i], 0.0};
    for(;i < n;i++)
        h[i] = complex_d{0.0, 0.0};
    FftForward(n, h.data());
    MagnitudeResponse(n, h.data(), r.data());
    for(i = 0;i < m;i++)
        mag[i] = Lerp(mag[i], r[i], f);
}

// Process the list of sources in the data set definition.
static int ProcessSources(TokenReaderT *tr, HrirDataT *hData)
{
    uint channels = (hData->mChannelType == CT_STEREO) ? 2 : 1;
    hData->mHrirsBase.resize(channels * hData->mIrCount * hData->mIrSize);
    double *hrirs = hData->mHrirsBase.data();
    std::vector<double> hrir(hData->mIrPoints);
    uint line, col, fi, ei, ai;
    int count;

    printf("Loading sources...");
    fflush(stdout);
    count = 0;
    while(TrIsOperator(tr, "["))
    {
        double factor[2]{ 1.0, 1.0 };

        TrIndication(tr, &line, &col);
        TrReadOperator(tr, "[");

        if(TrIsOperator(tr, "*"))
        {
            SourceRefT src;
            struct MYSOFA_EASY *sofa;
            uint si;

            TrReadOperator(tr, "*");
            if(!TrReadOperator(tr, "]") || !TrReadOperator(tr, "="))
                return 0;

            TrIndication(tr, &line, &col);
            if(!ReadSofaRef(tr, &src))
                return 0;

            if(hData->mChannelType == CT_STEREO)
            {
                char type[MAX_IDENT_LEN+1];
                ChannelTypeT channelType;

                if(!TrReadIdent(tr, MAX_IDENT_LEN, type))
                    return 0;

                channelType = MatchChannelType(type);

                switch(channelType)
                {
                    case CT_NONE:
                        TrErrorAt(tr, line, col, "Expected a channel type.\n");
                        return 0;
                    case CT_MONO:
                        src.mChannel = 0;
                        break;
                    case CT_STEREO:
                        src.mChannel = 1;
                        break;
                }
            }
            else
            {
                char type[MAX_IDENT_LEN+1];
                ChannelTypeT channelType;

                if(!TrReadIdent(tr, MAX_IDENT_LEN, type))
                    return 0;

                channelType = MatchChannelType(type);
                if(channelType != CT_MONO)
                {
                    TrErrorAt(tr, line, col, "Expected a mono channel type.\n");
                    return 0;
                }
                src.mChannel = 0;
            }

            sofa = LoadSofaFile(&src, hData->mIrRate, hData->mIrPoints);
            if(!sofa) return 0;

            for(si = 0;si < sofa->hrtf->M;si++)
            {
                printf("\rLoading sources... %d of %d", si+1, sofa->hrtf->M);
                fflush(stdout);

                float aer[3] = {
                    sofa->hrtf->SourcePosition.values[3*si],
                    sofa->hrtf->SourcePosition.values[3*si + 1],
                    sofa->hrtf->SourcePosition.values[3*si + 2]
                };
                mysofa_c2s(aer);

                if(std::fabs(aer[1]) >= 89.999f)
                    aer[0] = 0.0f;
                else
                    aer[0] = std::fmod(360.0f - aer[0], 360.0f);

                for(fi = 0;fi < hData->mFdCount;fi++)
                {
                    double delta = aer[2] - hData->mFds[fi].mDistance;
                    if(std::abs(delta) < 0.001) break;
                }
                if(fi >= hData->mFdCount)
                    continue;

                double ef{(90.0 + aer[1]) / 180.0 * (hData->mFds[fi].mEvCount - 1)};
                ei = static_cast<uint>(std::round(ef));
                ef = (ef - ei) * 180.0 / (hData->mFds[fi].mEvCount - 1);
                if(std::abs(ef) >= 0.1)
                    continue;

                double af{aer[0] / 360.0 * hData->mFds[fi].mEvs[ei].mAzCount};
                ai = static_cast<uint>(std::round(af));
                af = (af - ai) * 360.0 / hData->mFds[fi].mEvs[ei].mAzCount;
                ai = ai % hData->mFds[fi].mEvs[ei].mAzCount;
                if(std::abs(af) >= 0.1)
                    continue;

                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                if(azd->mIrs[0] != nullptr)
                {
                    TrErrorAt(tr, line, col, "Redefinition of source [ %d, %d, %d ].\n", fi, ei, ai);
                    return 0;
                }

                ExtractSofaHrir(sofa, si, 0, src.mOffset, hData->mIrPoints, hrir.data());
                azd->mIrs[0] = &hrirs[hData->mIrSize * azd->mIndex];
                azd->mDelays[0] = AverageHrirOnset(hData->mIrRate, hData->mIrPoints, hrir.data(), 1.0, azd->mDelays[0]);
                AverageHrirMagnitude(hData->mIrPoints, hData->mFftSize, hrir.data(), 1.0, azd->mIrs[0]);

                if(src.mChannel == 1)
                {
                    ExtractSofaHrir(sofa, si, 1, src.mOffset, hData->mIrPoints, hrir.data());
                    azd->mIrs[1] = &hrirs[hData->mIrSize * (hData->mIrCount + azd->mIndex)];
                    azd->mDelays[1] = AverageHrirOnset(hData->mIrRate, hData->mIrPoints, hrir.data(), 1.0, azd->mDelays[1]);
                    AverageHrirMagnitude(hData->mIrPoints, hData->mFftSize, hrir.data(), 1.0, azd->mIrs[1]);
                }

                // TODO: Since some SOFA files contain minimum phase HRIRs,
                // it would be beneficial to check for per-measurement delays
                // (when available) to reconstruct the HRTDs.
            }

            continue;
        }

        if(!ReadIndexTriplet(tr, hData, &fi, &ei, &ai))
            return 0;
        if(!TrReadOperator(tr, "]"))
            return 0;
        HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];

        if(azd->mIrs[0] != nullptr)
        {
            TrErrorAt(tr, line, col, "Redefinition of source.\n");
            return 0;
        }
        if(!TrReadOperator(tr, "="))
            return 0;

        for(;;)
        {
            SourceRefT src;

            if(!ReadSourceRef(tr, &src))
                return 0;

            // TODO: Would be nice to display 'x of y files', but that would
            // require preparing the source refs first to get a total count
            // before loading them.
            ++count;
            printf("\rLoading sources... %d file%s", count, (count==1)?"":"s");
            fflush(stdout);

            if(!LoadSource(&src, hData->mIrRate, hData->mIrPoints, hrir.data()))
                return 0;

            uint ti{0};
            if(hData->mChannelType == CT_STEREO)
            {
                char ident[MAX_IDENT_LEN+1];

                if(!TrReadIdent(tr, MAX_IDENT_LEN, ident))
                    return 0;
                ti = static_cast<uint>(MatchTargetEar(ident));
                if(static_cast<int>(ti) < 0)
                {
                    TrErrorAt(tr, line, col, "Expected a target ear.\n");
                    return 0;
                }
            }
            azd->mIrs[ti] = &hrirs[hData->mIrSize * (ti * hData->mIrCount + azd->mIndex)];
            azd->mDelays[ti] = AverageHrirOnset(hData->mIrRate, hData->mIrPoints, hrir.data(), 1.0 / factor[ti], azd->mDelays[ti]);
            AverageHrirMagnitude(hData->mIrPoints, hData->mFftSize, hrir.data(), 1.0 / factor[ti], azd->mIrs[ti]);
            factor[ti] += 1.0;
            if(!TrIsOperator(tr, "+"))
                break;
            TrReadOperator(tr, "+");
        }
        if(hData->mChannelType == CT_STEREO)
        {
            if(azd->mIrs[0] == nullptr)
            {
                TrErrorAt(tr, line, col, "Missing left ear source reference(s).\n");
                return 0;
            }
            else if(azd->mIrs[1] == nullptr)
            {
                TrErrorAt(tr, line, col, "Missing right ear source reference(s).\n");
                return 0;
            }
        }
    }
    printf("\n");
    for(fi = 0;fi < hData->mFdCount;fi++)
    {
        for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                if(azd->mIrs[0] != nullptr)
                    break;
            }
            if(ai < hData->mFds[fi].mEvs[ei].mAzCount)
                break;
        }
        if(ei >= hData->mFds[fi].mEvCount)
        {
            TrError(tr, "Missing source references [ %d, *, * ].\n", fi);
            return 0;
        }
        hData->mFds[fi].mEvStart = ei;
        for(;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];

                if(azd->mIrs[0] == nullptr)
                {
                    TrError(tr, "Missing source reference [ %d, %d, %d ].\n", fi, ei, ai);
                    return 0;
                }
            }
        }
    }
    for(uint ti{0};ti < channels;ti++)
    {
        for(fi = 0;fi < hData->mFdCount;fi++)
        {
            for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
            {
                for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
                {
                    HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];

                    azd->mIrs[ti] = &hrirs[hData->mIrSize * (ti * hData->mIrCount + azd->mIndex)];
                }
            }
        }
    }
    if(!TrLoad(tr))
    {
        mysofa_cache_release_all();
        return 1;
    }

    TrError(tr, "Errant data at end of source list.\n");
    mysofa_cache_release_all();
    return 0;
}


bool LoadDefInput(std::istream &istream, const char *startbytes, std::streamsize startbytecount,
    const char *filename, const uint fftSize, const uint truncSize, const ChannelModeT chanMode,
    HrirDataT *hData)
{
    TokenReaderT tr{istream};

    TrSetup(startbytes, startbytecount, filename, &tr);
    if(!ProcessMetrics(&tr, fftSize, truncSize, chanMode, hData)
        || !ProcessSources(&tr, hData))
        return false;

    return true;
}
