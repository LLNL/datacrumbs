#include <zlib.h>

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Compresses input data using zlib
std::vector<uint8_t> compress_zlib(const std::vector<uint8_t>& data) {
  // Use zlib's deflate with gzip header
  z_stream strm = {};
  int res = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
  assert(res == Z_OK);

  std::vector<uint8_t> compressed;
  compressed.resize(compressBound(data.size()));

  strm.next_in = const_cast<Bytef*>(data.data());
  strm.avail_in = data.size();
  strm.next_out = compressed.data();
  strm.avail_out = compressed.size();

  do {
    res = deflate(&strm, Z_FINISH);
    if (res == Z_OK) {
      // Output buffer was too small, increase size and continue
      size_t old_size = compressed.size();
      compressed.resize(old_size * 2);
      strm.next_out = compressed.data() + old_size;
      strm.avail_out = old_size;
    }
  } while (res == Z_OK);

  assert(res == Z_STREAM_END);

  compressed.resize(strm.total_out);
  deflateEnd(&strm);
  return compressed;
}

// Decompresses input data using zlib (gzip compatible)
std::vector<uint8_t> decompress_zlib(const std::vector<uint8_t>& compressed, size_t original_size) {
  z_stream strm = {};
  int res = inflateInit2(&strm, 15 + 16);  // 15 window bits + 16 for gzip
  assert(res == Z_OK);

  std::vector<uint8_t> decompressed(original_size);

  strm.next_in = const_cast<Bytef*>(compressed.data());
  strm.avail_in = compressed.size();
  strm.next_out = decompressed.data();
  strm.avail_out = decompressed.size();

  res = inflate(&strm, Z_FINISH);
  assert(res == Z_STREAM_END || res == Z_OK);

  decompressed.resize(strm.total_out);
  inflateEnd(&strm);
  return decompressed;
}

// Read file into vector using C APIs
std::vector<uint8_t> read_file(const std::string& filename) {
  FILE* infile = fopen(filename.c_str(), "rb");
  assert(infile != nullptr);
  fseek(infile, 0, SEEK_END);
  size_t size = ftell(infile);
  fseek(infile, 0, SEEK_SET);
  std::vector<uint8_t> buffer(size);
  size_t read = fread(buffer.data(), 1, size, infile);
  assert(read == size);
  fclose(infile);
  return buffer;
}

// Write vector to file using C APIs
void write_file(const std::string& filename, const std::vector<uint8_t>& data) {
  FILE* outfile = fopen(filename.c_str(), "wb");
  assert(outfile != nullptr);
  setvbuf(outfile, nullptr, _IOFBF, 8192);  // Enable line buffering
  size_t written = fwrite(data.data(), 1, data.size(), outfile);
  assert(written == data.size());
  fclose(outfile);
}

int main(int argc, char* argv[]) {
  if (argc == 4 && std::string(argv[1]) == "compress") {
    // Usage: compress <input_file> <output_file>
    std::vector<uint8_t> input_data = read_file(argv[2]);
    std::vector<uint8_t> compressed = compress_zlib(input_data);
    write_file(argv[3], compressed);
    std::cout << "File compressed: " << argv[2] << " -> " << argv[3] << std::endl;
    return 0;
  }

  if (argc == 5 && std::string(argv[1]) == "decompress") {
    // Usage: decompress <input_file> <output_file> <original_size>
    std::vector<uint8_t> compressed = read_file(argv[2]);
    size_t original_size = std::stoull(argv[4]);
    std::vector<uint8_t> decompressed = decompress_zlib(compressed, original_size);
    write_file(argv[3], decompressed);
    std::cout << "File decompressed: " << argv[2] << " -> " << argv[3] << std::endl;
    return 0;
  }

  if (argc == 4 && std::string(argv[1]) == "compress_str") {
    // Usage: compress_str <input_string> <output_file>
    std::string input_str = argv[2];
    std::vector<uint8_t> input_data(input_str.begin(), input_str.end());
    std::vector<uint8_t> compressed = compress_zlib(input_data);
    write_file(argv[3], compressed);
    std::cout << "String compressed and written to: " << argv[3] << std::endl;
    return 0;
  }

  if (argc == 4 && std::string(argv[1]) == "decompress_to_stdout") {
    // Usage: decompress_to_stdout <input_file> <original_size>
    std::vector<uint8_t> compressed = read_file(argv[2]);
    size_t original_size = std::stoull(argv[3]);
    std::vector<uint8_t> decompressed = decompress_zlib(compressed, original_size);
    std::cout << std::string(decompressed.begin(), decompressed.end()) << std::endl;
    return 0;
  }

  std::cerr << "Usage:\n"
            << "  " << argv[0] << " compress <input_file> <output_file>\n"
            << "  " << argv[0] << " decompress <input_file> <output_file> <original_size>\n"
            << "  " << argv[0] << " compress_str <input_string> <output_file>\n"
            << "  " << argv[0] << " decompress_to_stdout <input_file> <original_size>\n";
  return 1;
}