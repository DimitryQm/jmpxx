// SPDX-License-Identifier: MIT
// Bridge between jmpxx errors and the standard <system_error> facility,
// std::error_code.
//
// Two directions serve a codebase that already speaks std::error_code:
//
//   Adopt a foreign error_code: a function returning std::error_code, or
//   std::expected<T, std::error_code>, is carried into jmpxx without loss as a
//   result<T, std::error_code>, because the transport is generic over its error
//   type. The error_code travels verbatim, so its category identity and value are
//   preserved exactly. jmpxx::fail(ec) builds the failure, and the std::expected
//   form bridges through jmpxx/interop/expected.hpp. This is the lossless path for
//   an error_code that originates outside jmpxx, and it needs nothing from this
//   header beyond the documentation of it.
//
//   Expose a jmpxx error: a jmpxx::error is presented to error_code-based code as
//   a std::error_code in a jmpxx-owned category. The error's code becomes the
//   error_code value and the error's domain selects the category, so the pair
//   round-trips losslessly back to the same jmpxx::error. to_error_code performs
//   the conversion, make_error_code is the same operation under the name argument-
//   dependent lookup expects, and from_error_code recovers the jmpxx::error from a
//   code in a jmpxx category.
//
// This is a hosted extension. <system_error> is not part of the freestanding
// subset, so this header is never reached from jmpxx/core.hpp and is not usable in
// a freestanding build; a freestanding consumer that needs error_code interop does
// not have <system_error> to interoperate with in the first place.
//
// Category identity caveat: a std::error_category is identified by the address of
// its singleton. The jmpxx categories below are held in one process-wide table, so
// within a single binary two conversions of the same jmpxx error compare equal.
// Across a shared-library boundary built with hidden visibility, a category can be
// duplicated, the same hazard that affects std::error_category and typeid across
// modules; a program that compares jmpxx-derived error_codes across such a boundary
// should compare on (value, domain) recovered through from_error_code rather than
// on category identity.
#ifndef JMPXX_INTEROP_ERROR_CODE_HPP
#define JMPXX_INTEROP_ERROR_CODE_HPP

#include "jmpxx/core/config.hpp"
#include "jmpxx/core/error.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>

namespace jmpxx {

namespace detail {

// One std::error_category per jmpxx domain. The domain is part of the category
// identity rather than folded into the value, so the (code, domain) pair survives
// the round-trip and the value keeps the full int range. The category renders a
// generic message because a bare jmpxx code carries no per-value text the library
// can name; a program that wants richer text uses the rich or type-erased policy.
class jmpxx_category final : public std::error_category {
 public:
  explicit jmpxx_category(int domain) noexcept : domain_(domain) {}
  [[nodiscard]] int domain() const noexcept { return domain_; }

  [[nodiscard]] const char* name() const noexcept override { return "jmpxx"; }
  [[nodiscard]] std::string message(int value) const override {
    return "jmpxx error " + std::to_string(value) + " (domain " +
           std::to_string(domain_) + ")";
  }

 private:
  int domain_;
};

// The process-wide table of jmpxx categories. by_domain hands out a stable
// category per domain so repeated conversions share identity, and by_address lets
// from_error_code recognize a category as jmpxx-owned and recover its domain. Both
// are guarded by one mutex, and the categories live for the program's lifetime so
// every error_code that names one stays valid.
struct category_table {
  std::mutex m;
  std::map<int, std::unique_ptr<jmpxx_category>> by_domain;
  std::map<const std::error_category*, int> by_address;

  const jmpxx_category& get(int domain) {
    std::lock_guard<std::mutex> lock(m);
    auto it = by_domain.find(domain);
    if (it == by_domain.end()) {
      auto inserted =
          by_domain.emplace(domain, std::make_unique<jmpxx_category>(domain));
      it = inserted.first;
      by_address.emplace(it->second.get(), domain);
    }
    return *it->second;
  }

  // The domain of a category if it is jmpxx-owned, or -1 if it is foreign. -1 is
  // not a valid table key, so it is an unambiguous "not ours".
  int domain_of(const std::error_category& cat) {
    std::lock_guard<std::mutex> lock(m);
    auto it = by_address.find(&cat);
    return it == by_address.end() ? -1 : it->second;
  }
};

inline category_table& categories() noexcept {
  static category_table table;
  return table;
}

}  // namespace detail

// The jmpxx error category for a domain, default the generic domain 0. Its identity
// is stable for the program's lifetime.
[[nodiscard]] inline const std::error_category& error_category(
    int domain = 0) noexcept {
  return detail::categories().get(domain);
}

// jmpxx::error -> std::error_code, in the jmpxx category for the error's domain.
// make_error_code is the same conversion under the name argument-dependent lookup
// finds, so a jmpxx::error flows into an error_code-typed slot without naming this
// function.
[[nodiscard]] inline std::error_code to_error_code(error e) noexcept {
  return std::error_code(e.code, error_category(e.domain));
}
[[nodiscard]] inline std::error_code make_error_code(error e) noexcept {
  return to_error_code(e);
}

// std::error_code -> jmpxx::error. A code in a jmpxx category recovers its original
// code and domain exactly. A foreign code keeps its value and is tagged with the
// generic domain, which is lossy for the foreign category; a program that must
// preserve a foreign error_code carries it verbatim as result<T, std::error_code>
// rather than narrowing it here. is_jmpxx distinguishes the two so the caller can
// branch on which case it has.
[[nodiscard]] inline bool is_jmpxx(const std::error_code& ec) noexcept {
  return detail::categories().domain_of(ec.category()) >= 0;
}
[[nodiscard]] inline error from_error_code(const std::error_code& ec) noexcept {
  int domain = detail::categories().domain_of(ec.category());
  return domain >= 0 ? error(ec.value(), domain) : error(ec.value(), 0);
}

}  // namespace jmpxx

#endif  // JMPXX_INTEROP_ERROR_CODE_HPP
