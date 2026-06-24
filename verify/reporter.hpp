// SPDX-License-Identifier: MIT
// A tiny reporter for the verification harness. Each probe builds one report
// with named metrics and notes and renders it either human-readable or as one
// JSON object per line, so continuous integration consumes results without
// parsing prose.
#ifndef JMPXX_VERIFY_REPORTER_HPP
#define JMPXX_VERIFY_REPORTER_HPP

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace jv {

enum class Fmt { human, json };

class Report {
 public:
  Report(Fmt fmt, std::string probe) : fmt_(fmt), probe_(std::move(probe)) {}

  Report& num(const std::string& key, long long v) {
    metrics_.emplace_back(key, std::to_string(v));
    return *this;
  }
  Report& boolean(const std::string& key, bool v) {
    metrics_.emplace_back(key, v ? "true" : "false");
    return *this;
  }
  Report& str(const std::string& key, const std::string& v) {
    metrics_.emplace_back(key, quote(v));
    return *this;
  }
  Report& note(const std::string& n) {
    notes_.push_back(n);
    return *this;
  }
  void fail(const std::string& why) {
    ok_ = false;
    notes_.push_back(why);
  }
  bool ok() const { return ok_; }

  // Render and return a process exit code (0 = pass).
  int finish() {
    if (fmt_ == Fmt::json) {
      std::string s = "{\"probe\":" + quote(probe_) +
                      ",\"ok\":" + (ok_ ? "true" : "false") + ",\"metrics\":{";
      for (std::size_t i = 0; i < metrics_.size(); ++i) {
        if (i) s += ",";
        s += quote(metrics_[i].first) + ":" + metrics_[i].second;
      }
      s += "},\"notes\":[";
      for (std::size_t i = 0; i < notes_.size(); ++i) {
        if (i) s += ",";
        s += quote(notes_[i]);
      }
      s += "]}";
      std::printf("%s\n", s.c_str());
    } else {
      std::printf("[%s] %s\n", probe_.c_str(), ok_ ? "PASS" : "FAIL");
      for (const auto& m : metrics_)
        std::printf("    %-28s %s\n", m.first.c_str(), m.second.c_str());
      for (const auto& n : notes_) std::printf("    note: %s\n", n.c_str());
    }
    return ok_ ? 0 : 1;
  }

 private:
  static std::string quote(const std::string& s) {
    std::string o = "\"";
    for (char c : s) {
      if (c == '"' || c == '\\') o += '\\';
      if (c == '\n') {
        o += "\\n";
        continue;
      }
      o += c;
    }
    o += '"';
    return o;
  }

  Fmt fmt_;
  std::string probe_;
  bool ok_ = true;
  std::vector<std::pair<std::string, std::string>> metrics_;
  std::vector<std::string> notes_;
};

}  // namespace jv

#endif  // JMPXX_VERIFY_REPORTER_HPP
