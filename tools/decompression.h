#pragma once
#include <zlib.h>

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

constexpr size_t CHUNK_SIZE = 16 * 1024 * 1024;  // 16MB
namespace datacrumbs {
class GzipChunkReader {
 public:
  GzipChunkReader(const std::string& filename)
      : infile_(fopen(filename.c_str(), "rb")), strm_({}), eof_(false) {
    if (!infile_) throw std::runtime_error("Failed to open file: " + filename);
    if (inflateInit2(&strm_, 16 + MAX_WBITS) != Z_OK)
      throw std::runtime_error("Failed to initialize zlib inflate.");
    inbuf_.resize(CHUNK_SIZE);
    outbuf_.resize(CHUNK_SIZE);
  }

  ~GzipChunkReader() {
    inflateEnd(&strm_);
    if (infile_) fclose(infile_);
  }

  // Returns false when no more data is available
  bool nextChunk(std::string& chunk) {
    if (eof_) return false;

    strm_.avail_in = fread(inbuf_.data(), 1, inbuf_.size(), infile_);
    if (ferror(infile_)) throw std::runtime_error("Error reading input file.");
    if (strm_.avail_in == 0) {
      eof_ = true;
      return false;
    }
    strm_.next_in = reinterpret_cast<Bytef*>(inbuf_.data());

    chunk.clear();
    do {
      strm_.avail_out = outbuf_.size();
      strm_.next_out = reinterpret_cast<Bytef*>(outbuf_.data());
      int ret = inflate(&strm_, Z_NO_FLUSH);
      if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
        throw std::runtime_error("zlib inflate error.");
      size_t have = outbuf_.size() - strm_.avail_out;
      chunk.append(outbuf_.data(), have);
      if (ret == Z_STREAM_END) {
        eof_ = true;
        break;
      }
    } while (strm_.avail_out == 0);

    return !chunk.empty();
  }

 private:
  FILE* infile_;
  z_stream strm_;
  std::vector<char> inbuf_, outbuf_;
  bool eof_;
};
}  // namespace datacrumbs