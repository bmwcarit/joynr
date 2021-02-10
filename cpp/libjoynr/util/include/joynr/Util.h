/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <cstdint>
#include <ios>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/math/special_functions/next.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace joynr
{

namespace util
{

static const std::string SINGLE_LEVEL_WILDCARD("+");
static const std::string MULTI_LEVEL_WILDCARD("*");
static const std::string MULTICAST_PARTITION_SEPARATOR("/");

/**
 * @brief Check if the specified file exists and is readable.
 * @param filePath
 * @return true if file exists, false otherwise
 */
bool fileExists(const std::string& fileName);

std::string createMulticastId(const std::string& providerParticipantId,
                              const std::string& multicastName,
                              const std::vector<std::string>& partitions);

std::string extractParticipantIdFromMulticastId(const std::string& multicastId);

void validatePartitions(const std::vector<std::string>& partitions, bool allowWildcards = false);

/**
 * @brief check potential overflow which could happen when adding an offset to the address
 * @param address the memory address of the pointer
 * @param offset the offset to be added to the address
 * @return true if an overflow happens, false otherwise
 */
bool isAdditionOnPointerCausesOverflow(std::uintptr_t address, int offset);

/**
 * @brief MT-safe retrieval of string describing error number
 * @param errorNumber The error number for which string description should be retrieved
 * @return string describing error number
 */
std::string getErrorString(int errorNumber);

/**
 * @brief converts an attribute name to its getter function
 * @param attributeName name of the attribute; MUST NOT be an empty string!
 * @return name of getter function;
 * if the input value is 'foo' this method returns 'getFoo'
 */
std::string attributeGetterFromName(const std::string& attributeName);

/*
 * Return the content of fileName as a string.
 * It assumes the file exists and is accessible.
 */
std::string loadStringFromFile(const std::string& fileName);

/*
 * It opens a file with the provided flags and writes to it.
 * The file does not need to exist.
 */
void writeToFile(const std::string& fileName,
                 const std::string& strToSave,
                 std::ios_base::openmode mode);

/*
 * It saves strToSave to the specified fileName.
 * The file does not need to exists.
 */
void saveStringToFile(const std::string& fileName, const std::string& strToSave);

/*
 * It appends strToSave to the specified file.
 * The file does not need to exist.
 */
void appendStringToFile(const std::string& fileName, const std::string& strToSave);

/**
 * Create a Uuid for use in Joynr.
 *
 * This is simply a wrapper around boost::uuid
 */
std::string createUuid();

template <typename T>
std::set<T> vectorToSet(const std::vector<T>& v)
{
    return std::set<T>(v.begin(), v.end());
}

template <typename T>
bool setContainsSet(const std::set<T>& haystack, const std::set<T>& needles)
{
    return std::includes(haystack.cbegin(), haystack.cend(), needles.cbegin(), needles.cend());
}

template <typename T>
bool vectorContains(const std::vector<T>& v, const T& e)
{
    return v.end() != std::find(v.cbegin(), v.cend(), e);
}

template <typename T>
auto removeAll(std::vector<T>& v, const T& e)
{
    return v.erase(std::remove(v.begin(), v.end(), e), v.end());
}

/**
 *@brief this meta function allows to check whether a type U is derived from a template T
 */
template <template <typename...> class T, typename U>
struct IsDerivedFromTemplate
{
private:
    template <typename... Args>
    static decltype(static_cast<const T<Args...>&>(std::declval<U>()), std::true_type{}) test(
            const T<Args...>&);
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(IsDerivedFromTemplate::test(std::declval<U>()))::value;
};

template <typename CurrentIterator, typename EndIterator, typename Fun>
std::enable_if_t<std::is_same<CurrentIterator, EndIterator>::value, bool> invokeOnImpl(const Fun&)
{
    return false;
}

template <typename CurrentIterator, typename EndIterator, typename Fun>
std::enable_if_t<!std::is_same<CurrentIterator, EndIterator>::value, bool> invokeOnImpl(
        const Fun& fun)
{
    using CurrentType = typename boost::mpl::deref<CurrentIterator>::type;
    using NextIterator = typename boost::mpl::next<CurrentIterator>::type;
    if (fun(CurrentType{})) {
        return invokeOnImpl<NextIterator, EndIterator>(fun);
    }
    return true;
}

/**
 * @brief invoke a Function for each type of a boost::mpl Sequence.
 *
 * This will call fun(Ti{}) for each type Ti of the Sequence.
 * The iteration can be exited early by returning false from fun.
 */
template <typename Sequence, typename Fun>
bool invokeOn(const Fun& fun)
{
    using BeginIterator = typename boost::mpl::begin<Sequence>::type;
    using EndIterator = typename boost::mpl::end<Sequence>::type;
    return invokeOnImpl<BeginIterator, EndIterator>(fun);
}

template <typename T>
auto as_weak_ptr(std::shared_ptr<T> ptr)
{
    return std::weak_ptr<T>(ptr);
}

/**
 * calculate the number of Units in the Last Place between x and y
 */
template <typename FloatingPoint>
FloatingPoint getAbsFloatDistance(FloatingPoint x, FloatingPoint y)
{
    return std::abs(boost::math::float_distance(x, y));
}

static constexpr std::size_t MAX_ULPS = 4;

template <typename T>
std::enable_if_t<std::is_floating_point<T>::value, bool> compareValues(
        const T& x,
        const T& y,
        const std::size_t maxUlps = MAX_ULPS)
{
    return getAbsFloatDistance(x, y) <= maxUlps;
}

template <typename T>
std::enable_if_t<!std::is_floating_point<T>::value, bool> compareValues(
        const T& x,
        const T& y,
        const std::size_t maxUlps = MAX_ULPS)
{
    std::ignore = maxUlps;
    return x == y;
}

} // namespace util

} // namespace joynr
#endif // UTIL_H
