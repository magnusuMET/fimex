/*
 * Fimex
 *
 * (C) Copyright 2008, met.no
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
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <vector>
#include <utility>
#include <iterator>
#include <sstream>
#include <cmath>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <limits>
#include "fimex/CDMException.h"

namespace MetNoFimex
{
/**
 * Round a double to integer.
 */
int round(double num);

/**
 * Remove leading and trailing spaces.
 * @param str string to trim
 */
std::string trim(const std::string& str);

/**
 * Join values from an iterator to a string, using delimiter as separator.
 *
 * @param start
 * @param end
 * @param delim separator, default to ","
 */
template<class InputIterator>
std::string join(InputIterator start, InputIterator end, std::string delim = ",")
{
    if (start == end) return "";
    std::ostringstream buffer;
    InputIterator current = start++;
    while (start != end) {
        buffer << *current << delim;
        current = start++;
    }
    buffer << *current;
    return buffer.str();
}

/**
 * Find closest distinct elements in an unordered list. The order of elements is not defined.
 *
 * Except for the case where all elements are equal, it is always ensured that the neighbors
 * are distinct.

 * @param start
 * @param end
 * @return pair of the positions of a and b, with a closer than b
 */
template<typename InputIterator>
std::pair<typename std::iterator_traits<InputIterator>::difference_type, typename std::iterator_traits<InputIterator>::difference_type>
find_closest_distinct_elements(InputIterator start, InputIterator end, double x)
{
    using namespace std;
    typename iterator_traits<InputIterator>::difference_type retVal1 = 0;
    typename iterator_traits<InputIterator>::difference_type retVal2 = 0;
    InputIterator cur = start;
    typename iterator_traits<InputIterator>::value_type v1;
    double v1Diff, v2Diff;
    if (start != end) {
        v1 = *start;
        v1Diff = abs(x-*start);
        v2Diff = v1Diff;
    }
    while (cur != end) {
        double vDiff = fabs(x-*cur);
        if (vDiff <= v2Diff) {
            if (vDiff < v1Diff) {
                retVal2 = retVal1;
                v2Diff = v1Diff;
                v1 = *cur;
                retVal1 = distance(start, cur);
                v1Diff = vDiff;
            } else if (*cur != v1) {
                retVal2 = distance(start, cur);
                v2Diff = vDiff;
            }
        } // else nothing to be done
        cur++;
    }
    return make_pair<typename iterator_traits<InputIterator>::difference_type, typename iterator_traits<InputIterator>::difference_type>(retVal1, retVal2);
}

/**
 * Find closest distinct neighbor elements in an unordered list, with a <= x < b
 * It might extrapolate if x is smaller than all elements (or x > all elements) and
 * fall back to find_closest_distinct_elements()
 *
 * Except for the case where all elements are equal, it is always ensured that the neighbors
 * are distinct.

 * @param start
 * @param end
 * @return pair of the positions of a and b, with a closer than b
 */
template<typename InputIterator>
std::pair<typename std::iterator_traits<InputIterator>::difference_type, typename std::iterator_traits<InputIterator>::difference_type>
find_closest_neighbor_distinct_elements(InputIterator start, InputIterator end, double x)
{
    using namespace std;
    if (start == end)
        return make_pair<typename iterator_traits<InputIterator>::difference_type, typename iterator_traits<InputIterator>::difference_type>(0, 0);

    InputIterator lowest = start;
    InputIterator heighest = start;
    InputIterator cur = start;
    double lowDiff = x - *cur;
    double heighDiff = *cur -x;
    double maxDiff = std::numeric_limits<double>::max();
    if (lowDiff < 0)
        lowDiff = maxDiff;
    if (heighDiff < 0)
        heighDiff = maxDiff;
    while (++cur != end) {
        if (*cur <= x) {
            double diff = x - *cur;
            if (diff < lowDiff) {
                lowDiff = diff;
                lowest = cur;
            }
        } else {
            double diff = *cur - x;
            if (diff < heighDiff) {
                heighDiff = diff;
                heighest = cur;
            }
        }
    }
    if (lowDiff == maxDiff ||
            heighDiff == maxDiff) {
        // extrapolating
        return find_closest_distinct_elements(start, end, x);
    }

    return make_pair<typename iterator_traits<InputIterator>::difference_type, typename iterator_traits<InputIterator>::difference_type>(distance(start, lowest), distance(start, heighest));

}
/**
 * Join values from an iterator of pointers to a string, using delimiter as separator.
 *
 * @param start
 * @param end
 * @param delim separator, default to ","
 */
template<class InputIterator>
std::string joinPtr(InputIterator start, InputIterator end, std::string delim = ",")
{
    if (start == end) return "";
    std::ostringstream buffer;
    InputIterator current = start++;
    while (start != end) {
        buffer << **current << delim;
        current = start++;
    }
    buffer << **current;
    return buffer.str();
}

/**
 * Tokenize a string by a delimiter. This function will automaticall remove empty strings
 * at the beginning or anywhere inside the string.
 *
 * This function has been derived from http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
 * @param str the string to tokenize
 * @param delimiters the delimiters between the tokens. That can be multiple delimiters, i.e. whitespace is " \t\n\r"
 * @return vector of tokens
 */
std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters = " ");


/**
 * convert a string to lowercase
 */
std::string string2lowerCase(const std::string& str);

/**
 * convert a type (i.e. int, float) to string representation
 */
template<typename T>
std::string type2string(T in) {
	std::ostringstream buffer;
	buffer << in;
	return buffer.str();
}

/**
 * specialization for high prececision
 */
template<>
std::string type2string<double>(double in);


template<typename T>
T string2type(std::string s) {
	T retVal;
	std::stringstream buffer;
	buffer << s;
	buffer >> retVal;
	return retVal;
}

typedef long epoch_seconds;
/**
 * convert a posixTime to seconds sinc 1970-01-01
 * @param time time to convert
 */
epoch_seconds posixTime2epochTime(const boost::posix_time::ptime& time);

/**
 * convert a string with dots to a vector with type T
 * @param str f.e. 3.5,4.5,...,17.5
 * @param delimiter optional delimiter, defaults to ,
 */
template<typename T>
std::vector<T> tokenizeDotted(const std::string& str, const std::string& delimiter = ",") throw(CDMException)
{
	std::vector<std::string> tokens = tokenize(str, delimiter);
    std::vector<T> vals;
    for (std::vector<std::string>::iterator tok = tokens.begin(); tok != tokens.end(); ++tok) {
		std::string current = trim(*tok);
		if (current == "...") {
            size_t currentPos = vals.size();
            if (currentPos < 2) {
                throw CDMException("tokenizeDotted: cannot use ... expansion at position " + type2string(currentPos-1) +", need at least two values before");
            }
            T last = vals[currentPos-1];
            T dist = last - vals[currentPos-2];
            T curVal = last + dist;
            // positive if values get larger, negative if curVal gets samller
            double direction = (dist > 0) ? 1 : -1;
            if (++tok != tokens.end()) {
                T afterDotVal = string2type<T>(*tok);
                // expand the dots until before the afterDotVal, compare against rounding error
                double roundError = direction*dist*-1.e-5;
                while ((curVal - afterDotVal)*direction < roundError) {
                    vals.push_back(curVal);
                    curVal += dist;
                }
                // add the afterDotVal
                vals.push_back(afterDotVal);
            }
		} else {
			T val = string2type<T>(current);
			vals.push_back(val);
		}
	}
	return vals;
}

/** static_cast as a functor */
template<typename OUT>
struct staticCast {
    template<typename IN>
    OUT operator()(const IN& in) { return static_cast<OUT>(in); }
};


/**
 * Scale a value using fill, offset and scale
 */
template<typename IN, typename OUT>
class ScaleValue : public std::unary_function<IN, OUT>
{
private:
    IN oldFill_;
    double oldScale_;
    double oldOffset_;
    OUT newFill_;
    double newScale_;
    double newOffset_;
public:
    ScaleValue(double oldFill, double oldScale, double oldOffset, double newFill, double newScale, double newOffset) :
        oldFill_(static_cast<IN>(oldFill)), oldScale_(oldScale), oldOffset_(oldOffset),
        newFill_(static_cast<OUT>(newFill)), newScale_(newScale), newOffset_(newOffset) {}
    OUT operator()(const IN& in) const {
        if (in == oldFill_ || isinf(static_cast<double>(in))) {
            return newFill_;
        } else {
            return static_cast<OUT>(((oldScale_*in + oldOffset_)-newOffset_)/newScale_);
        }
    }
};

/**
 * Change the missing value
 */
template<typename IN, typename OUT>
class ChangeMissingValue : public std::unary_function<IN, OUT>
{
private:
    IN oldFill_;
    OUT newFill_;
public:
    ChangeMissingValue(double oldFill, double newFill) :
        oldFill_(static_cast<IN>(oldFill)), newFill_(static_cast<OUT>(newFill)) {}
    OUT operator()(const IN& in) const {
        if (in == oldFill_ || isinf(static_cast<double>(in))) {
            return newFill_;
        } else {
            return static_cast<OUT>(in);
        }
    }
};

}

#endif /*UTILS_H_*/
