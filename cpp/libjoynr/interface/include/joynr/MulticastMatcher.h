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
#ifndef MULTICASTMATCHER_H
#define MULTICASTMATCHER_H

#include <cstddef>
#include <regex>
#include <string>

namespace joynr
{

/*
 * The multicastId in input for the constructor is used to generate a pattern
 * which in turn is used to check if an incoming multicastId matches.
 * Also the mutlicastId is assumed to be valid: no validation are performed here.
 */
class MulticastMatcher
{
public:
    std::string _multicastId;
    std::regex _pattern;

    explicit MulticastMatcher(const std::string& _multicastId);
    bool doesMatch(const std::string& incomingMulticastId) const;
    bool operator==(const MulticastMatcher& other) const;

private:
    const std::string _regExpPlusSign;
    const std::string _regExpKleenStarSign;
};

class MulticastMatcherHash
{
public:
    std::size_t operator()(const MulticastMatcher& k) const
    {
        return std::hash<std::string>()(k._multicastId);
    }
};

} // namespace joynr
#endif // MULTICASTMATCHER_H
