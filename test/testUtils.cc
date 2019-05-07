/*
 * Fimex, testUtils.cc
 *
 * (C) Copyright 2009, met.no
 *
 * Project Info:  https://wiki.met.no/fimex/start
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 *  Created on: Oct 5, 2009
 *      Author: Heiko Klein
 */


#include "testinghelpers.h"

#include "fimex/Utils.h"

//#define COMPARE_WITH_BOOST_LEXICAL_CAST
#ifdef COMPARE_WITH_BOOST_LEXICAL_CAST
#include <boost/lexical_cast.hpp>
#endif

#include <cctype>
#include <numeric>
#include <type_traits>

using namespace std;
using namespace MetNoFimex;

TEST4FIMEX_TEST_CASE(test_scaleValue)
{
    ScaleValue<int, double> sv(-32686, 2, 0, -2.7e11, 0.5, 1);
    double delta = 1e-5;
    TEST4FIMEX_CHECK_CLOSE(-2., sv(0), delta);
    TEST4FIMEX_CHECK_CLOSE(-2.7e11, sv(-32686), delta);
    TEST4FIMEX_CHECK_CLOSE(2., sv(1), delta);
}

#ifndef HAVE_BOOST_UNIT_TEST_FRAMEWORK // seems to have problems with ostream << vector<string>

TEST4FIMEX_TEST_CASE(test_split)
{
    typedef std::vector<std::string> string_v;
    TEST4FIMEX_CHECK_EQ((string_v{"hei", "ho"}), split_any("hei ho", " "));
    TEST4FIMEX_CHECK_EQ((string_v{"hei", "ho"}), split_any("hei:ho", ":"));

    TEST4FIMEX_CHECK_EQ((string_v{"h:e:i", "h:o"}), split_any("h:e:i,h:o", ","));
    TEST4FIMEX_CHECK_EQ((string_v{"h", "e", "i,h", "o"}), split_any("h:e:i,h:o", ":"));

    TEST4FIMEX_CHECK_EQ((string_v{""}), split_any("", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"", ""}), split_any(":", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"hei"}), split_any("hei", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"hei", ""}), split_any("hei:", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"", "hei"}), split_any(":hei", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"hei", "", "ho"}), split_any("hei::ho", ":"));
}

TEST4FIMEX_TEST_CASE(test_tokenize)
{
    typedef std::vector<std::string> string_v;
    TEST4FIMEX_CHECK_EQ((string_v{"hei", "ho"}), tokenize("hei ho", " "));

    TEST4FIMEX_CHECK_EQ((string_v{"hei", "ho"}), tokenize("hei:ho", ":"));

    TEST4FIMEX_CHECK_EQ((string_v{"h:e:i", "h:o"}), tokenize("h:e:i,h:o", ","));
    TEST4FIMEX_CHECK_EQ((string_v{"h", "e", "i,h", "o"}), tokenize("h:e:i,h:o", ":"));

    TEST4FIMEX_CHECK_EQ((string_v{}), tokenize("", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{}), tokenize(":", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"hei"}), tokenize("hei", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"hei"}), tokenize("hei:", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"hei"}), tokenize(":hei", ":"));
    TEST4FIMEX_CHECK_EQ((string_v{"hei", "ho"}), tokenize("hei::ho", ":"));
}
#endif

TEST4FIMEX_TEST_CASE(test_tokenizeDotted)
{
    vector<int> pos = tokenizeDotted<int>("12");
    TEST4FIMEX_REQUIRE_EQ(1, pos.size());
    TEST4FIMEX_CHECK_EQ(pos[0], 12);

    string dotted = "1.2,2.4,...,6";
    vector<float> tokens = tokenizeDotted<float>(dotted);
    TEST4FIMEX_REQUIRE_EQ(5, tokens.size());
    TEST4FIMEX_CHECK_CLOSE(tokens[4], 6.f, .1);

    string dotted2 = "1.2,2.4,...,4.8";
    vector<float> tokens2 = tokenizeDotted<float>(dotted2);
    TEST4FIMEX_REQUIRE_EQ(4, tokens2.size());
    TEST4FIMEX_CHECK_CLOSE(tokens2[2], 3.6f, .1);

    // backwards
    string dotted3 = "6,4.8,...,-1.2";
    vector<double> tokens3 = tokenizeDotted<double>(dotted3);
    TEST4FIMEX_REQUIRE_EQ(7, tokens3.size());
    TEST4FIMEX_CHECK_CLOSE(tokens3[2], 3.6, .1);

    // old, now fixed error, due to using abs instead of fabs
    string dotted4 = "90,89.96,...,50";
    vector<double> tokens4 = tokenizeDotted<double>(dotted4);
    TEST4FIMEX_REQUIRE_EQ(40 * 25 + 1, tokens4.size());
    TEST4FIMEX_CHECK_CLOSE(tokens4.at(tokens4.size() - 2), 50.04, .1);

    // check in case end doesn't match forward
    string dotted5 = "10,20,...,35";
    vector<double> tokens5 = tokenizeDotted<double>(dotted5);
    TEST4FIMEX_CHECK_EQ(4, tokens5.size());

    // check in case end doesn't match backward
    string dotted6 = "20,10,...,-35";
    vector<double> tokens6 = tokenizeDotted<double>(dotted6);
    TEST4FIMEX_CHECK_EQ(7, tokens6.size());
}

TEST4FIMEX_TEST_CASE(test_find_closest_distinct_elements)
{
    int ary[8] = {1, 2, 3, 4, 4, -1, -2, 5};
    pair<size_t, size_t> p = find_closest_distinct_elements(&ary[0], &ary[0]+8, 1.5);
    TEST4FIMEX_CHECK_EQ(p.first, 0);
    TEST4FIMEX_CHECK_EQ(p.second, 1);

    p = find_closest_distinct_elements(&ary[0], &ary[0]+8, 4);
    TEST4FIMEX_CHECK_EQ(p.first, 3);
    TEST4FIMEX_CHECK_EQ(p.second, 7);

    p = find_closest_distinct_elements(&ary[0], &ary[0]+8, -1);
    TEST4FIMEX_CHECK_EQ(p.first, 5);
    TEST4FIMEX_CHECK_EQ(p.second, 6);

    p = find_closest_distinct_elements(&ary[0], &ary[0]+8, -3);
    TEST4FIMEX_CHECK_EQ(p.first, 6);
    TEST4FIMEX_CHECK_EQ(p.second, 5);

    p = find_closest_distinct_elements(&ary[0], &ary[0]+8, 5);
    TEST4FIMEX_CHECK_EQ(p.first, 7);
    TEST4FIMEX_CHECK_EQ(p.second, 3);

    // below follow test for find_closest_neighbor_distinct_elements
    p = find_closest_neighbor_distinct_elements(&ary[0], &ary[0]+8, 1.5);
    TEST4FIMEX_CHECK_EQ(p.first, 0);
    TEST4FIMEX_CHECK_EQ(p.second, 1);

    p = find_closest_neighbor_distinct_elements(&ary[0], &ary[0]+8, 4);
    TEST4FIMEX_CHECK_EQ(p.first, 3);
    TEST4FIMEX_CHECK_EQ(p.second, 7);

    p = find_closest_neighbor_distinct_elements(&ary[0], &ary[0]+8, -1);
    TEST4FIMEX_CHECK_EQ(p.first, 5);
    TEST4FIMEX_CHECK_EQ(p.second, 0);

    p = find_closest_neighbor_distinct_elements(&ary[0], &ary[0]+8, -3);
    TEST4FIMEX_CHECK_EQ(p.first, 6);
    TEST4FIMEX_CHECK_EQ(p.second, 5);

    p = find_closest_neighbor_distinct_elements(&ary[0], &ary[0]+8, 5);
    TEST4FIMEX_CHECK_EQ(p.first, 7);
    TEST4FIMEX_CHECK_EQ(p.second, 3);
}

TEST4FIMEX_TEST_CASE(test_regexEscape)
{
    TEST4FIMEX_CHECK_EQ(regex_escape("mouse"), "mouse");
    TEST4FIMEX_CHECK_EQ(regex_escape("d*g"), "d\\*g");
    TEST4FIMEX_CHECK_EQ(regex_escape("[]^$1234\\"), "\\[\\]\\^\\$1234\\\\");
}

TEST4FIMEX_TEST_CASE(test_scanFiles)
{
    vector<string> files;
    scanFiles(files, topSrcDir(), -1, std::regex(".*stUti.?.?\\.cc"), true);
    TEST4FIMEX_REQUIRE_EQ(files.size(), 1);
    TEST4FIMEX_CHECK(files.at(0).find("testUtils.cc") != string::npos);
    files.clear();
    scanFiles(files, topSrcDir(), -1, std::regex(".*stUti.?.?\\.cc"), false);
    TEST4FIMEX_REQUIRE_EQ(files.size(), 1);
    TEST4FIMEX_CHECK(files.at(0).find("testUtils.cc") != string::npos);
}

TEST4FIMEX_TEST_CASE(test_globFiles)
{
    vector<string> files;
    string glob = topSrcDir() + "/**stUti??.cc";
    globFiles(files, glob);
    TEST4FIMEX_REQUIRE_EQ(files.size(), 1);
    TEST4FIMEX_CHECK(files.at(0).find("testUtils.cc") != string::npos);
    files.clear();

    string glob2 = topSrcDir() + "/test/*stUti??.cc";
    globFiles(files, glob2);
    TEST4FIMEX_REQUIRE_EQ(files.size(), 1);
    TEST4FIMEX_CHECK(files.at(0).find("testUtils.cc") != string::npos);
}

TEST4FIMEX_TEST_CASE(test_type2string)
{
    TEST4FIMEX_CHECK_EQ("123", type2string((char)123));
    TEST4FIMEX_CHECK_EQ("234", type2string((unsigned char)234));

    TEST4FIMEX_CHECK_EQ("12345", type2string((short)12345));
    TEST4FIMEX_CHECK_EQ("34567", type2string((unsigned short)34567));

    TEST4FIMEX_CHECK_EQ("fimex", type2string("fimex"));
}

TEST4FIMEX_TEST_CASE(test_starts_ends_width)
{
    TEST4FIMEX_CHECK(!starts_with("", "fimex"));
    TEST4FIMEX_CHECK(!starts_with("fi", "fimex"));
    TEST4FIMEX_CHECK(starts_with("fimex", "fim"));
    TEST4FIMEX_CHECK(starts_with("fimex", "fimex"));
    TEST4FIMEX_CHECK(!starts_with("fimex", "no"));

    TEST4FIMEX_CHECK(!ends_with("", "fimex"));
    TEST4FIMEX_CHECK(!ends_with("ex", "fimex"));
    TEST4FIMEX_CHECK(ends_with("fimex", "mex"));
    TEST4FIMEX_CHECK(ends_with("fimex", "fimex"));
    TEST4FIMEX_CHECK(!ends_with("fimex", "no"));
}

TEST4FIMEX_TEST_CASE(test_trim)
{
    TEST4FIMEX_CHECK_EQ("x", trim("   x"));
    TEST4FIMEX_CHECK_EQ("x", trim("x "));
    TEST4FIMEX_CHECK_EQ("x", trim("   x "));
    TEST4FIMEX_CHECK_EQ("x", trim("   x"));
    TEST4FIMEX_CHECK_EQ("x", trim("x "));

    TEST4FIMEX_CHECK_EQ("hei", trim("   hei "));
    TEST4FIMEX_CHECK_EQ("hei", trim("   hei"));
    TEST4FIMEX_CHECK_EQ("hei", trim("hei "));

    TEST4FIMEX_CHECK_EQ("hei hei", trim("  hei hei"));
    TEST4FIMEX_CHECK_EQ("", trim("    "));
}

TEST4FIMEX_TEST_CASE(test_replace_all_copy)
{
    TEST4FIMEX_CHECK_EQ("hei hei", replace_all_copy("hek hek", 'k', 'i'));
    TEST4FIMEX_CHECK_EQ("hei hei", replace_all_copy("hei hei", 'k', 'i'));
}

#define CHECK_THROW_S2T(T, S) \
    TEST4FIMEX_CHECK_THROW(string2type<T>(S), std::runtime_error)
#define CHECK_EQ_S2T(T, V, S) \
    TEST4FIMEX_CHECK_EQ((T)V, string2type<T>(S))

#ifdef COMPARE_WITH_BOOST_LEXICAL_CAST
#define CHECK_THROW_BLC(T, S) \
    TEST4FIMEX_CHECK_THROW(boost::lexical_cast<T>(S), boost::bad_lexical_cast)
#define CHECK_EQ_BLC(T, V, S) \
    TEST4FIMEX_CHECK_EQ((T)V, boost::lexical_cast<T>(S))
#else
#define CHECK_THROW_BLC(T, S) \
    do { } while (0) // nothing
#define CHECK_EQ_BLC(T, V, S) \
    do { } while (0) // nothing
#endif

#define CHECK_THROW(T, S) \
    do { CHECK_THROW_BLC(T, S); CHECK_THROW_S2T(T, S); } while (0)
#define CHECK_EQ(T, V, S) \
    do { CHECK_EQ_BLC(T, V, S); CHECK_EQ_S2T(T, V, S); } while (0)

TEST4FIMEX_TEST_CASE(test_string2type)
{
    TEST4FIMEX_CHECK_EQ("hei", string2type<std::string>("hei"));

    TEST4FIMEX_CHECK_EQ((char)23, string2type<char>("23"));
    TEST4FIMEX_CHECK_EQ((unsigned char)34, string2type<unsigned char>("34"));
    TEST4FIMEX_CHECK_EQ((char)-12, string2type<char>("-12"));

    CHECK_EQ(int, 12, "12");
    CHECK_EQ(float, 1234.5f, "1234.5");
    CHECK_EQ(double, 0.125, ".125"); // no leading 0
    CHECK_EQ(int, -23, "-23");

    CHECK_THROW(int, "   1234");
    CHECK_THROW(int, "1111e");
    CHECK_THROW(float, "123e");
    CHECK_THROW(unsigned char, "222  ");
    CHECK_THROW(int, ".25e");
    CHECK_THROW(int, "125e0");

    CHECK_THROW(int, "55.5");
    CHECK_THROW(int, "hei");
}
