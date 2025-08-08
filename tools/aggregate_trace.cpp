#include <json-c/json.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "decompression.h"

using namespace std;

struct Stats {
  int count = 0;
  double sum = 0;
  double sum_sq = 0;
  int min = INT_MAX;
  int max = INT_MIN;

  void add(int dur) {
    count++;
    sum += dur;
    sum_sq += dur * dur;
    if (dur < min) min = dur;
    if (dur > max) max = dur;
  }

  double mean() const { return count ? sum / count : 0; }

  double stddev() const {
    if (count < 2) return 0;
    double m = mean();
    return sqrt((sum_sq / count) - (m * m));
  }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <gzipped_jsonlines_file>" << endl;
    return 1;
  }

  // Prepare output file path
  std::filesystem::path input_path(argv[1]);
  std::string basename = input_path.stem().string();
  std::filesystem::path output_dir = std::filesystem::absolute(
      std::filesystem::path(argv[0]).parent_path().parent_path() / "tools/output");
  std::filesystem::create_directories(output_dir);
  std::filesystem::path output_path = output_dir / (basename + ".csv");

  ofstream outfile(output_path);
  if (!outfile) {
    cerr << "Failed to open output file: " << output_path << endl;
    return 1;
  }

  map<pair<string, string>, Stats> stats_map;

  try {
    datacrumbs::GzipChunkReader reader(argv[1]);
    bool first_line = true;
    std::string chunk, leftover;
    while (reader.nextChunk(chunk)) {
      leftover += chunk;
      size_t pos = 0;
      while (true) {
        size_t newline = leftover.find('\n', pos);
        if (newline == std::string::npos) break;
        std::string line = leftover.substr(pos, newline - pos);
        pos = newline + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (first_line) {
          first_line = false;
          if (line == "[") continue;
        }
        if (line == "]") continue;
        if (line.empty()) continue;

        struct json_object* jobj = json_tokener_parse(line.c_str());
        if (!jobj) {
          cerr << "Skipping invalid JSON: " << line << endl;
          continue;
        }

        struct json_object *jcat, *jname, *jdur;
        if (!json_object_object_get_ex(jobj, "cat", &jcat) ||
            !json_object_object_get_ex(jobj, "name", &jname) ||
            !json_object_object_get_ex(jobj, "dur", &jdur)) {
          json_object_put(jobj);
          continue;
        }

        string cat = json_object_get_string(jcat);
        string name = json_object_get_string(jname);
        int dur = json_object_get_int(jdur);

        stats_map[{cat, name}].add(dur);

        json_object_put(jobj);
      }
      leftover = leftover.substr(pos);
    }
    // Handle any remaining data in leftover
    if (!leftover.empty()) {
      std::string line = leftover;
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line != "]" && !line.empty()) {
        struct json_object* jobj = json_tokener_parse(line.c_str());
        if (jobj) {
          struct json_object *jcat, *jname, *jdur;
          if (json_object_object_get_ex(jobj, "cat", &jcat) &&
              json_object_object_get_ex(jobj, "name", &jname) &&
              json_object_object_get_ex(jobj, "dur", &jdur)) {
            string cat = json_object_get_string(jcat);
            string name = json_object_get_string(jname);
            int dur = json_object_get_int(jdur);
            stats_map[{cat, name}].add(dur);
          }
          json_object_put(jobj);
        }
      }
    }
  } catch (const std::exception& ex) {
    cerr << ex.what() << endl;
    return 1;
  }

  // Collect stats into a vector for sorting by sum (total duration)
  vector<pair<pair<string, string>, Stats>> stats_vec(stats_map.begin(), stats_map.end());
  std::sort(stats_vec.begin(), stats_vec.end(), [](const auto& a, const auto& b) {
    if (a.second.count != b.second.count) return a.second.count < b.second.count;
    return a.second.sum < b.second.sum;
  });
  ostream& out = outfile;
  // CSV header
  out << "index,cat,name,count,min,max,mean,stddev,sum" << endl;

  int index = 0;
  for (const auto& [key, stats] : stats_vec) {
    out << index++ << "," << '"' << key.first << "\"," << '"' << key.second << "\"," << stats.count
        << "," << stats.min << "," << stats.max << "," << fixed << setprecision(2) << stats.mean()
        << "," << fixed << setprecision(2) << stats.stddev() << "," << fixed << setprecision(2)
        << stats.sum << endl;
  }

  cout << "Output written to: " << output_path << endl;
  return 0;
}