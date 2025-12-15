#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include "generate_parameter_lib_validators/parameter_validators.hpp"

namespace gplv = generate_parameter_lib_validators;

static bool ok( const rclcpp::Parameter &p )
{
  return static_cast<bool>( gplv::valid_topic_name( p ) );
}

static std::string err( const rclcpp::Parameter &p )
{
  auto res = gplv::valid_topic_name( p );
  if ( res ) {
    return "";
  }
  return res.error();
}

TEST( ValidTopicName, AcceptsRelativeNames )
{
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "foo" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "foo_bar" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "foo1" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "foo/bar" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "_foo/_bar_1" ) ) ) );
}

TEST( ValidTopicName, AcceptsAbsoluteNamesStartingWithSlash )
{
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "/foo" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "/foo/bar" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "/_foo/_bar_1" ) ) ) );
}

TEST( ValidTopicName, AcceptsPrivateNamesStartingWithTilde )
{
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "~" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "~/foo" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "~/foo/bar" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "~/_foo/_bar_1" ) ) ) );
}

TEST( ValidTopicName, RejectsInvalidCharacters )
{
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo:bar" ) ) ) ); // colon
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo-bar" ) ) ) ); // dash
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo bar" ) ) ) ); // space
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo.bar" ) ) ) ); // dot
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo@bar" ) ) ) ); // at
}

TEST( ValidTopicName, RejectsEmptySegmentsAndTrailingSlash )
{
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "/" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo/" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "/foo/" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo//bar" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "//foo" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "~/" ) ) ) );     // "~/" ends with '/'
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "~//foo" ) ) ) ); // empty segment after "~"
}

TEST( ValidTopicName, RejectsSegmentsStartingWithDigit )
{
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "1foo" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo/1bar" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "/1foo" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "~/1foo" ) ) ) );
}

TEST( ValidTopicName, AllowsDigitsAfterFirstCharacterInSegment )
{
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "foo1" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "foo/bar2" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "/foo1/bar2" ) ) ) );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", std::string( "~/foo1/bar2" ) ) ) );
}

TEST( ValidTopicName, EnforcesTildePlacementRules )
{
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "f~oo" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "/~foo" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "foo/~bar" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "~~/foo" ) ) ) );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", std::string( "~foo" ) ) ) ); // must be "~" or "~/..."
}

TEST( ValidTopicName, EnforcesLengthLimit256 )
{
  // Build a valid absolute name of length exactly 256.
  // Start with "/a" then pad with "/a" segments until length matches.
  std::string s = "/a";
  while ( s.size() + 2 <= 256U ) { // +2 for "/a"
    s += "/a";
  }
  // If we overshot by 1 (should not), trim safely (but keep valid)
  if ( s.size() > 256U ) {
    s.resize( 256U );
    // Ensure not ending with '/'
    if ( !s.empty() && s.back() == '/' ) {
      s.back() = 'a';
    }
  }
  ASSERT_EQ( s.size(), 256U );
  EXPECT_TRUE( ok( rclcpp::Parameter( "t", s ) ) ) << err( rclcpp::Parameter( "t", s ) );

  // One more char -> invalid
  std::string too_long = s + "a";
  ASSERT_EQ( too_long.size(), 257U );
  EXPECT_FALSE( ok( rclcpp::Parameter( "t", too_long ) ) );
}

TEST( ValidTopicName, WorksForStringArrayAndFailsOnAnyInvalidEntry )
{
  {
    std::vector<std::string> v = { "foo", "/bar_baz", "~/qux1" };
    EXPECT_TRUE( ok( rclcpp::Parameter( "t", v ) ) ) << err( rclcpp::Parameter( "t", v ) );
  }
  {
    std::vector<std::string> v = { "foo", "bar//baz", "qux" };
    EXPECT_FALSE( ok( rclcpp::Parameter( "t", v ) ) );
  }
  {
    std::vector<std::string> v = { "foo", "bar", "1bad" };
    EXPECT_FALSE( ok( rclcpp::Parameter( "t", v ) ) );
  }
  {
    std::vector<std::string> v = { "foo", "bar", "bad:char" };
    EXPECT_FALSE( ok( rclcpp::Parameter( "t", v ) ) );
  }
}
