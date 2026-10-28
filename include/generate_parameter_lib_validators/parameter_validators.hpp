#ifndef GENERATE_PARAMETER_LIB_VALIDATORS_GENERATE_PARAMETER_LIB_VALIDATORS_HPP
#define GENERATE_PARAMETER_LIB_VALIDATORS_GENERATE_PARAMETER_LIB_VALIDATORS_HPP

#include <cctype>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <tl/expected.hpp>

namespace generate_parameter_lib_validators
{
namespace detail
{

inline bool is_valid_segment_char( unsigned char c )
{
  return ( std::isalpha( c ) != 0 ) || ( std::isdigit( c ) != 0 ) || ( c == '_' );
}

inline bool is_valid_topic_name_strict( const std::string &name, std::string *reason = nullptr )
{
  if ( name.empty() ) {
    if ( reason )
      *reason = "name is empty";
    return false;
  }

  if ( name.size() > 256U ) {
    if ( reason )
      *reason = "name exceeds 256 characters";
    return false;
  }

  // Handle "~" prefix rules:
  // - "~" may appear only as the first character
  // - "~" alone is allowed
  // - "~/" is allowed (private namespace root)
  // - "~" must not appear anywhere else
  std::size_t i = 0;
  bool allow_leading_slash = true;

  if ( name[0] == '~' ) {
    // Ensure no other '~' later
    for ( std::size_t k = 1; k < name.size(); ++k ) {
      if ( name[k] == '~' ) {
        if ( reason )
          *reason = "'~' may only be the first character";
        return false;
      }
    }

    if ( name.size() == 1 ) {
      return true; // "~" ok
    }

    if ( name[1] != '/' ) {
      if ( reason )
        *reason = "if '~' is used, it must be followed by '/' or be alone";
      return false;
    }

    // Start parsing after "~"
    i = 1; // points at '/'
    allow_leading_slash = true;
  } else {
    // Ensure no '~' anywhere
    for ( char c : name ) {
      if ( c == '~' ) {
        if ( reason )
          *reason = "'~' may only be the first character";
        return false;
      }
    }
  }

  if ( name.back() == '/' ) {
    if ( reason )
      *reason = "name must not end with '/'";
    return false;
  }

  // Parsing state
  bool at_segment_start = true;
  bool in_segment = false;

  // Leading '/' is allowed (absolute topic), but it must not create an empty segment later
  if ( allow_leading_slash && i < name.size() && name[i] == '/' ) {
    ++i;
    at_segment_start = true;
    in_segment = false;
  }

  for ( ; i < name.size(); ++i ) {
    const unsigned char c = static_cast<unsigned char>( name[i] );

    if ( c == '/' ) {
      // separator: previous segment must not be empty
      if ( !in_segment ) {
        if ( reason )
          *reason = "empty segment (consecutive '/' or leading '/' without a segment)";
        return false;
      }
      at_segment_start = true;
      in_segment = false;
      continue;
    }

    // Only allow alnum/underscore in segments
    if ( !is_valid_segment_char( c ) ) {
      if ( reason )
        *reason = "contains invalid character (only A-Z a-z 0-9 '_' '/' allowed, and optional "
                  "leading '~')";
      return false;
    }

    if ( at_segment_start ) {
      // Segment may not start with a digit
      if ( std::isdigit( c ) != 0 ) {
        if ( reason )
          *reason = "segment starts with a digit";
        return false;
      }
      // Segment must start with alpha or underscore
      if ( !( ( std::isalpha( c ) != 0 ) || c == '_' ) ) {
        if ( reason )
          *reason = "segment must start with a letter or underscore";
        return false;
      }
      at_segment_start = false;
      in_segment = true;
      continue;
    }

    // Non-first segment chars: already validated by is_valid_segment_char
    in_segment = true;
  }

  if ( !in_segment ) {
    if ( reason )
      *reason = "empty last segment";
    return false;
  }

  return true;
}

inline bool is_valid_relative_topic_name_strict( const std::string &name,
                                                 std::string *reason = nullptr )
{
  if ( !name.empty() && ( name.front() == '/' || name.front() == '~' ) ) {
    if ( reason )
      *reason = "name must be relative (must not start with '/' or '~')";
    return false;
  }
  return is_valid_topic_name_strict( name, reason );
}

inline tl::expected<void, std::string> validate_one( const std::string &value,
                                                     const std::string &param_name,
                                                     const std::string &where,
                                                     bool relative_only = false )
{
  std::string reason;
  const bool valid = relative_only ? is_valid_relative_topic_name_strict( value, &reason )
                                   : is_valid_topic_name_strict( value, &reason );
  if ( !valid ) {
    return tl::make_unexpected(
        "Invalid topic name " + where + " for parameter '" + param_name + "': '" + value +
        "'. Reason: " + reason +
        ". Rules: segments separated by '/', each segment [A-Za-z_][A-Za-z0-9_]*, "
        "no empty segments, " +
        std::string( relative_only ? "must not start with '/' or '~', "
                                   : "may start with '/' (absolute), may start with '~/' "
                                     "(private), '~' only as first char, " ) +
        "must not end with '/', max length 256." );
  }
  return {};
}

inline tl::expected<void, std::string> validate_topic_parameter( const rclcpp::Parameter &parameter,
                                                                 const std::string &validator_name,
                                                                 bool relative_only )
{
  using PT = rclcpp::ParameterType;

  switch ( parameter.get_type() ) {
  case PT::PARAMETER_STRING: {
    const auto &v = parameter.as_string();
    return validate_one( v, parameter.get_name(), "", relative_only );
  }

  case PT::PARAMETER_STRING_ARRAY: {
    const auto values = parameter.as_string_array();
    for ( std::size_t i = 0; i < values.size(); ++i ) {
      if ( auto res = validate_one( values[i], parameter.get_name(),
                                    "at index " + std::to_string( i ), relative_only );
           !res ) {
        return res;
      }
    }
    return {};
  }

  default:
    return tl::make_unexpected( validator_name +
                                " only supports string or string_array parameters, but '" +
                                parameter.get_name() + "' has a different type." );
  }
}

} // namespace detail

inline tl::expected<void, std::string> valid_topic_name( const rclcpp::Parameter &parameter )
{
  return detail::validate_topic_parameter( parameter, "valid_topic_name", false );
}

// Like valid_topic_name, but rejects absolute ('/foo') and private ('~', '~/foo') names.
// Useful for names that are appended to another namespace, e.g. "~/" + name.
inline tl::expected<void, std::string> valid_relative_topic_name( const rclcpp::Parameter &parameter )
{
  return detail::validate_topic_parameter( parameter, "valid_relative_topic_name", true );
}

} // namespace generate_parameter_lib_validators

#endif // GENERATE_PARAMETER_LIB_VALIDATORS_GENERATE_PARAMETER_LIB_VALIDATORS_HPP
