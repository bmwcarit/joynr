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

#include "joynr/MulticastMatcher.h"

#include "joynr/Util.h"

namespace joynr
{

MulticastMatcher::MulticastMatcher(const std::string& multicastId)
        : multicastId(multicastId),
          pattern(),
          regExpPlusSign(R"([a-zA-Z0-9]+)"),
          regExpKleenStarSign(R"((/[a-zA-Z0-9]+)*)")
{
    std::string multicasIdPattern = multicastId;

    // traverse the string and transform any + into a regex matching a sequence of
    // alphanumeric characters
    size_t pos = 0;
    while ((pos = multicasIdPattern.find(joynr::util::SINGLE_LEVEL_WILDCARD, pos)) !=
           std::string::npos) {
        multicasIdPattern.replace(pos, 1, regExpPlusSign);
        pos += regExpPlusSign.length();
    }

    // transform '*' into a regex matching a sequence of alphanumeric characters longer
    // then 1 separated by a forward slash '/'
    // also, use knowledge that * can only appear at the end of the multicastId
    if (multicastId.back() == joynr::util::MULTI_LEVEL_WILDCARD[0]) {
        multicasIdPattern.replace(multicasIdPattern.length() - 2, 2, regExpKleenStarSign);
    }

    multicasIdPattern.insert(0, 1, '^');
    multicasIdPattern.append("$");

    pattern.assign(multicasIdPattern);
}

bool MulticastMatcher::doesMatch(const std::string& incomingMulticastId) const
{
    return std::regex_search(incomingMulticastId, pattern);
}

bool MulticastMatcher::operator==(const MulticastMatcher& other) const
{
    return multicastId == other.multicastId;
}

} // joynr
