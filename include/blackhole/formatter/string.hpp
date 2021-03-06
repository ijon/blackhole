#pragma once

#include <functional>
#include <memory>
#include <string>

#include "blackhole/formatter.hpp"
#include "blackhole/severity.hpp"

namespace blackhole {
inline namespace v1 {

class record_t;
class writer_t;

template<typename>
struct factory;

}  // namespace v1
}  // namespace blackhole

namespace blackhole {
inline namespace v1 {
namespace config {

class node_t;

}  // namespace config
}  // namespace v1
}  // namespace blackhole

namespace blackhole {
inline namespace v1 {
namespace formatter {
namespace token {

class token_t;
class option_t;

}  // namespace string
}  // namespace formatter
}  // namespace v1
}  // namespace blackhole


namespace blackhole {
inline namespace v1 {
namespace formatter {

/// Severity mapping function.
///
/// Default value just writes an integer representation.
///
/// \param severity an integer representation of current log severity.
/// \param spec the format specification as it was provided with the initial pattern.
/// \param writer result writer.
typedef std::function<void(int severity, const std::string& spec, writer_t& writer)> severity_map;

namespace string {

/// Represents configuration for a single optional placeholder.
///
/// Optional placeholders allows to nicely format some patterns where there are non-reserved
/// attributes used and its presents is undetermined. Unlike required placeholders it does not throw
/// an exception if there are no attribute with the given name in the set.
/// Also it provides additional functionality with optional surrounding a present attribute with
/// some prefix and suffix.
struct optional_t {
    /// Prefix string that will be prepended before an associated attribute if it exists.
    std::string prefix;

    /// Suffix string that will be appended after an associated attribute if it exists.
    std::string suffix;
};

/// Represents configuration for a single leftover placeholder.
struct leftover_t {
    /// Represents filtering policy for duplicated attributes in placeholder.
    enum class filter_t {
        /// No filtering, allow duplicate attributes.
        none,
        /// Filter duplicate attributes locally, meaning that there will be only the last attribute
        /// with unique name in the given leftover placeholder.
        local
    };

    /// Filtering policy.
    filter_t filter;

    /// Prefix string that will be prepended before a placeholder if there are at least one
    /// non-reserved attribute in a set.
    std::string prefix;

    /// Suffix string that will be appended after a placeholder if there are at least one
    /// non-reserved attribute in a set.
    std::string suffix;

    /// Key-value pattern.
    std::string pattern;

    /// Separator between each attribute representation.
    std::string separator;
};

}  // namespace string

/// The string formatter is responsible for effective converting the given record to a string using
/// precompiled pattern and options.
///
/// This formatter allows to specify the pattern using python-like syntax with braces and attribute
/// names.
///
/// For example, the given pattern `{severity:d}, [{timestamp}]: {message}` would result in
/// something like this: `1, [2015-11-18 15:50:12.630953]: HTTP1.1 - 200 OK`.
///
/// Let's explain what's going on when a log record passed such pattern.
///
/// There are three named arguments or attributes: severity, timestamp and message. The severity
/// is represented as signed integer, because of `:d` format specifier. Other tho arguments haven't
/// such specifiers, so they are represented using default types for each attribute. In our case
/// the timestamp and message attributes are formatted as strings.
///
/// Considering message argument almost everything is clear, but for timestamp there are internal
/// magic comes. There is default `%Y-%m-%d %H:%M:%S.%f` pattern for timestamp attributes which
/// reuses `strftime` standard placeholders with an extension of microseconds - `%f`. It means
/// that the given timestamp is formatted with 4-digit decimal year, 1-12 decimal month and so on.
///
/// See \ref http://en.cppreference.com/w/cpp/chrono/c/strftime for more details.
///
/// The formatter uses python-like syntax with all its features, like aligning, floating point
/// precision etc.
///
/// For example the `{re:+.3f}; {im:+.6f}` pattern is valid and results in `+3.140; -3.140000`
/// message with `re: 3.14` and `im: -3.14` attributes provided.
///
/// For more information see \ref http://cppformat.github.io/latest/syntax.html resource.
///
/// With a few predefined exceptions the formatter supports all userspace attributes. The exceptions
/// are: message, severity, timestamp, process and thread. For these attributes there are special
/// rules and it's impossible to override then even with the same name attribute.
///
/// For message attribute there are no special rules. It's still allowed to extend the specification
/// using fill, align and other specifiers.
///
/// With timestamp attribute there is an extension of either explicitly providing formatting pattern
/// or forcing the attibute to be printed as an integer.
/// In the first case for example the pattern may be declared as `{timestamp:{%Y}s}` which results
/// in only year formattinh using YYYY style.
/// In the second case one can force the timestamp to be printed as microseconds passed since epoch.
///
/// Severity attribute can be formatted either as an integer or using the provided callback with the
/// following signature: `auto(int, writer_t&) -> void` where the first argument means an actual
/// severity level, the second one - streamed writer.
///
/// Process attribute can be represented as either an PID or process name using `:d` and `:s` types
/// respectively: `{process:s}` and `{process:d}`.
///
/// At last the thread attribute can be formatted as either thread id in platform-independent hex
/// representation by default or explicitly with `:x` type, thread id in platform-dependent way
/// using `:d` type or as a thread name if specified, nil otherwise.
///
/// The formatter will throw an exception if an attribute name specified in pattern won't be found
/// in the log record. Of course Blackhole catches this, but it results in dropping the entire
/// message.
/// To avoid this the formatter supports optional generic attributes, which can be specified using
/// the `optional_t` option with an optional prefix and suffix literals printed if an attribute
/// exists.
/// For example an `{id}` pattern with the `[` prefix and `]` suffix options results in empty
/// message if there is no `source` attribute in the record, `[42]` otherwise (where id = 42).
///
/// Blackhole also supports the leftover placeholder starting with `...` and meaning to print all
/// userspace attributes in a reverse order they were provided.
///
/// # Performance
///
/// Internally the given pattern is compiled into the list of tokens during construction time. All
/// further operations are performed using that list to achieve maximum performance.
///
/// All visited tokens are written directly into the given writer instance with an internal small
/// stack-allocated buffer, growing using the heap on overflow.
class string_t : public formatter_t {
    class inner_t;
    std::unique_ptr<inner_t, auto(*)(inner_t*) -> void> inner;

public:
    explicit string_t(const std::string& pattern);
    explicit string_t(const std::string& pattern, severity_map sevmap);

    /// Configuration.

    auto set(const std::string& name, const string::optional_t& option) -> void;
    auto set(const std::string& name, const string::leftover_t& option) -> void;

    /// Formatting.

    auto format(const record_t& record, writer_t& writer) -> void;
};

}  // namespace formatter

template<>
struct factory<formatter::string_t> {
    static auto type() -> const char*;
    static auto from(const config::node_t& config) -> formatter::string_t;
};

}  // namespace v1
}  // namespace blackhole
