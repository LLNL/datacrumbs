#pragma once
// Generated Headers
#include <datacrumbs/datacrumbs_config.h>
// Other headers
#include <datacrumbs/common/logging.h>
// std headers
#include <zlib.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace datacrumbs {
class ZlibCompression {
 public:
  ZlibCompression(const std::string& output_file, size_t chunk_size)
      : output_file_(output_file), chunk_size_(chunk_size), buffer_(chunk_size) {
    file_ = std::fopen(output_file_.c_str(), "wb");
    if (!file_) {
      throw std::runtime_error("Failed to open output file for writing");
    }
    strm_.zalloc = Z_NULL;
    strm_.zfree = Z_NULL;
    strm_.opaque = Z_NULL;
    if (deflateInit2(&strm_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) !=
        Z_OK) {
      std::fclose(file_);
      throw std::runtime_error("Failed to initialize zlib for gzip compression");
    }
    buffer_offset_ = 0;
  }

  ~ZlibCompression() {}

  void finalize() {
    flush();
    deflateEnd(&strm_);
    if (file_) {
      std::fclose(file_);
    }
  }

  void compress(const std::string& data) {
    strm_.avail_in = static_cast<uInt>(data.size());
    strm_.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

    while (strm_.avail_in > 0) {
      strm_.avail_out = static_cast<uInt>(chunk_size_ - buffer_offset_);
      strm_.next_out = reinterpret_cast<Bytef*>(&buffer_[buffer_offset_]);
      int ret = deflate(&strm_, Z_NO_FLUSH);
      if (ret != Z_OK && ret != Z_STREAM_END) {
        throw std::runtime_error("Compression failed");
      }
      size_t have = chunk_size_ - buffer_offset_ - strm_.avail_out;
      buffer_offset_ += have;
      if (buffer_offset_ == chunk_size_) {
        write_chunk();
      }
    }
  }

  void flush() {
    int ret;
    do {
      strm_.avail_out = static_cast<uInt>(chunk_size_ - buffer_offset_);
      strm_.next_out = reinterpret_cast<Bytef*>(&buffer_[buffer_offset_]);
      ret = deflate(&strm_, Z_FINISH);
      size_t have = chunk_size_ - buffer_offset_ - strm_.avail_out;
      buffer_offset_ += have;
      if (buffer_offset_ == chunk_size_) {
        write_chunk();
      }
    } while (ret != Z_STREAM_END);

    if (buffer_offset_ > 0) {
      write_chunk();
    }
  }

 private:
  void write_chunk() {
    if (buffer_offset_ > 0 && file_) {
      if (std::fwrite(buffer_.data(), 1, buffer_offset_, file_) != buffer_offset_) {
        perror("Failed to write compressed chunk to file");
      }
      buffer_offset_ = 0;
    }
  }

  std::string output_file_;
  size_t chunk_size_;
  std::vector<uint8_t> buffer_;
  size_t buffer_offset_;
  FILE* file_;
  z_stream strm_;
};
}  // namespace datacrumbs