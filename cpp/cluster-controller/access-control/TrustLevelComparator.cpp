/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "TrustLevelComparator.h"
#include <cassert>

namespace joynr
{

using namespace infrastructure;

int TrustLevelComparator::compare(infrastructure::QtTrustLevel::Enum a,
                                  infrastructure::QtTrustLevel::Enum b)
{
    int ordinalA = trustLevelOrdinal(a);
    int ordinalB = trustLevelOrdinal(b);

    if (ordinalA == ordinalB) {
        return 0;
    } else if (ordinalA > ordinalB) {
        return 1;
    } else {
        return -1;
    }
}

int TrustLevelComparator::trustLevelOrdinal(QtTrustLevel::Enum trustLevel)
{
    switch (trustLevel) {
    case QtTrustLevel::NONE:
        return 0;
    case QtTrustLevel::LOW:
        return 1;
    case QtTrustLevel::MID:
        return 2;
    case QtTrustLevel::HIGH:
        return 3;
    default:
        assert(false);
        return -1;
    }
}

} // namespace joynr
