/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <cstddef>
#include <algorithm>
#include <chrono>
#include <iterator>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <memory>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next_prior.hpp>

namespace joynr
{

class Logger;

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
  * Splits a byte array representation of multiple JSON objects into
  * a list of byte arrays, each containing a single JSON object.
  */
std::vector<std::string> splitIntoJsonObjects(const std::string& jsonStream);

std::string attributeGetterFromName(const std::string& attributeName);

/*
 * Return the content of fileName as a string.
 * It assumes the file exists and is accessible.
 */
std::string loadStringFromFile(const std::string& fileName);

/*
 * It saves strToSave to the specified fileName.
 * The file does not need to exists.
 */
void saveStringToFile(const std::string& fileName, const std::string& strToSave);

/**
 * Create a Uuid for use in Joynr.
 *
 * This is simply a wrapper around boost::uuid
 */
std::string createUuid();

/**
 * Log a serialized Joynr message
 */
void logSerializedMessage(Logger& logger,
                          const std::string& explanation,
                          const std::string& message);

template <typename T>
std::set<T> vectorToSet(const std::vector<T>& v)
{
    return std::set<T>(v.begin(), v.end());
}

template <typename T>
bool setContainsSet(const std::set<T>& haystack, const std::set<T>& needle)
{
    bool contains = true;
    for (const T& element : haystack) {
        contains = (needle.count(element) == 1);
    }
    return contains;
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

template <template <typename...> class MultiMap, typename K, typename V>
std::size_t removeAllPairsFromMultiMap(MultiMap<K, V>& map, const std::pair<K, V>& pair)
{
    std::size_t removedElements = 0;
    auto range = map.equal_range(pair.first);
    auto it = range.first;
    while (it != range.second) {
        if (it->second == pair.second) {
            it = map.erase(it);
            ++removedElements;
        } else {
            ++it;
        }
    }
    return removedElements;
}

template <typename Map>
auto getKeyVectorForMap(const Map& map)
{
    std::vector<typename Map::key_type> keys;
    boost::copy(map | boost::adaptors::map_keys, std::back_inserter(keys));
    return keys;
}

/**
 * Converts a std::chrono::system_clock::time_point to milliseconds
 */
std::uint64_t toMilliseconds(const std::chrono::system_clock::time_point& timePoint);

/**
 * Converts a std::chrono::system_clock::time_point to a printable string
 */
std::string toDateString(const std::chrono::system_clock::time_point& timePoint);

template <typename T>
auto as_weak_ptr(std::shared_ptr<T> ptr)
{
    return std::weak_ptr<T>(ptr);
}

} // namespace util

} // namespace joynr
#endif // UTIL_H
