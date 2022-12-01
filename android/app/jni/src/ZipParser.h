/*
Copyright (c) 2022 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#pragma once

#include <istream>
#include <vector>
#include <zlib.h>

/**
 * Parses a zip file. Only supports Deflate algorithm, via libz.
 * Limited to 64 meg files, due to non-streaming api.
 */
class ZipParser
{
public:
   ZipParser(std::istream& in): m_in(in)
   {
      // all fields are in little-endian (non-network) order
      in.seekg(0, std::ios::end);
      m_file_size = in.tellg();
      if (m_file_size < 22)
      {
         m_error = "Too small to be a zip file";
         return;
      }

      // last record should be an end of central directory, which is 22 bytes
      // plus up to 64k comment. Read the last 64k + 22 bytes, and scan
      // backwards for the signature.
      uint8_t buffer[65536 + 22];
      uint64_t buffer_size = std::min((uint64_t)(65558 + 22), m_file_size);
      in.seekg(-buffer_size, std::ios::end);
      in.read((char*)buffer, buffer_size);
      uint64_t eocd_pos = 0;
      for (int64_t i = buffer_size - 22; i >= 0; --i)
      {
         if (*(uint32_t*)&buffer[i] == 0x06054b50)
         {
            // normal ZIP EOCD
            eocd_pos = i + m_file_size - buffer_size;
            m_central_directory_entries = *(uint16_t*)&buffer[i + 10];
            m_central_directory_size = *(uint32_t*)&buffer[i + 12];
            m_central_directory_pos = *(uint32_t*)&buffer[i + 16];
            break;
         }
      }
      if (m_central_directory_pos == 0 ||
          m_central_directory_pos > m_file_size ||
          m_central_directory_pos + m_central_directory_size > m_file_size)
      {
         m_error = "Not a zip file";
         return;
      }
   }

   bool Valid() const { return m_error.empty(); }
   const std::string& Error() const { return m_error; }

   class iterator;

   class ZipEntry
   {
   public:
      const std::string& Name() const { return m_name; }
      uint64_t Size() const { return m_header.uncompressed_size; }
      // TODO: streaming api?
      std::vector<uint8_t> Contents()
      {
         if (!Valid() ||
             m_in == nullptr ||
             m_header.uncompressed_size == 0)
         {
            return {};
         }
         // Put a cap on the maximum size (since this isn't a streaming api)
         if (m_header.uncompressed_size > 64 * 1024 * 1024)
         {
            m_error = "Zip file too big";
            return {};
         }
         struct
         {
            uint32_t magic;
            uint16_t version, flags, compression_method;
            uint32_t modification_time, checksum, compressed_size, uncompressed_size;
            uint16_t name_size, extra_size;
         } __attribute__((packed)) local_header;
         m_in->seekg(m_header.disk_offset);
         m_in->read((char*)&local_header, sizeof(local_header));
         // Make sure this matches the directory entry
         if (local_header.magic != 0x04034b50)
         {
            m_error = "Corrupt zip headers";
            return {};
         }
         m_in->seekg(local_header.extra_size + local_header.name_size, std::ios::cur);
         std::vector<uint8_t> ret;
         if (m_header.compression_method == 0)
         {
            // no compression. file just stored.
            ret.resize(m_header.uncompressed_size);
            if (!m_in->read((char*)ret.data(), ret.size()))
            {
               m_error = "EOF while reading from zip file";
               return {};
            }
         }
         else if (m_header.compression_method == 8)
         {
            // DEFLATE, ie libz
            // for a raw stream, zlib needs an extra byte to finish its state
            std::vector<uint8_t> compressed(m_header.compressed_size + 1);
            if (!m_in->read((char*)compressed.data(), m_header.compressed_size))
            {
               m_error = "EOF while reading from zip file";
               return {};
            }
            z_stream strm{};
            // -MAX_WBITS indicates this is a raw deflate stream
            int status = inflateInit2(&strm, -MAX_WBITS);
            if (status != Z_OK)
            {
               m_error = "Unable to initialize zlib";
               return {};
            }
            ret.resize(m_header.uncompressed_size);
            strm.avail_in = compressed.size();
            strm.next_in = compressed.data();
            strm.avail_out = ret.size();
            strm.next_out = ret.data();
            status = inflate(&strm, Z_FINISH);

            // did it succeed?
            if (status != Z_STREAM_END ||
                strm.avail_out != 0)
            {
               m_error = "Unable to decompress data";
               return {};
            }
         }
         else
         {
            m_error = "Unsupported zip compression method";
            return {};
         }
         // validate checksum
         uint64_t crc = crc32(0, nullptr, 0);
         crc = crc32(crc, ret.data(), ret.size());
         if (crc != m_header.checksum)
         {
            m_error = "Bad checksum in zipfile";
            return {};
         }
         return ret;
      }

      bool Valid() const { return m_error.empty(); }
      const std::string& Error() const { return m_error; }

   private:
      ZipEntry(): m_header{} {}
      bool Read(std::istream* in)
      {
         m_error.clear();
         m_in = in;
         if (!in)
         {
            return false;
         }
         if (!in->read((char*)&m_header, sizeof(m_header)))
         {
            return false;
         }
         if (m_header.magic != 0x02014b50)
         {
            m_error = "Invalid zip directory";
            return false;
         }
         char buffer[m_header.name_size];
         if (!in->read(buffer, m_header.name_size))
         {
            m_header.magic = 0;
            return false;
         }
         m_name.assign(buffer, m_header.name_size);
         return true;
      }

      uint64_t NextOffset() const
      {
         return 46 +
                m_header.name_size +
                m_header.extra_size +
                m_header.comment_size;
      }

   private:
      struct {
         uint32_t magic, version;
         uint16_t flags, compression_method;
         uint32_t modification_time, checksum, compressed_size, uncompressed_size;
         uint16_t name_size, extra_size, comment_size, disk, internal;
         uint32_t external, disk_offset;
      } __attribute__((packed)) m_header;
      std::string m_name;
      std::istream* m_in = nullptr;
      std::string m_error;

      friend class ZipParser::iterator;
   };

   class iterator
   {
   public:
      iterator(std::istream* in, uint64_t offset, uint64_t offset_max):
         m_in(in), m_offset(offset), m_offset_max(offset_max)
      {
         if (m_offset < m_offset_max)
         {
            m_in->seekg(offset);
            m_ref.Read(m_in);
         }
      }
      ~iterator() {}
      bool operator==(const iterator& o) const { return m_offset == o.m_offset && m_offset_max == o.m_offset_max; }
      bool operator!=(const iterator& o) const { return !(*this == o); }
      iterator& operator=(const iterator& o) { m_in = o.m_in; m_offset = o.m_offset; m_offset_max = o.m_offset_max; return *this;}
      iterator& operator++()
      {
         m_offset += m_ref.NextOffset();
         if (m_offset >= m_offset_max)
         {
            m_offset = m_offset_max;
         }
         else
         {
            m_in->seekg(m_offset);
            if (!m_ref.Read(m_in))
            {
               // Read failed. mark the iterator as end()
               m_offset = m_offset_max;
            }
         }
         return *this;
      }
      ZipEntry& operator*() { return m_ref; }

   private:
      std::istream* m_in;
      uint64_t m_offset = 0;
      uint64_t m_offset_max = 0;

      ZipEntry m_ref;
   };

   iterator begin() { return iterator(&m_in, m_central_directory_pos, m_central_directory_pos + m_central_directory_size); }
   iterator end() { return iterator(&m_in, m_central_directory_pos + m_central_directory_size, m_central_directory_pos + m_central_directory_size); }

private:
   std::istream& m_in;
   std::string m_error;

   uint64_t m_file_size = 0;
   uint64_t m_central_directory_pos = 0;
   uint64_t m_central_directory_size = 0;
   uint64_t m_central_directory_entries = 0;
};