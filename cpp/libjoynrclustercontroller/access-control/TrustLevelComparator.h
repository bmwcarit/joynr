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

#ifndef TRUSTLEVELCOMPARATOR_H
#define TRUSTLEVELCOMPARATOR_H

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"

namespace joynr
{

/**
 *
 */
class JOYNRCLUSTERCONTROLLER_EXPORT TrustLevelComparator
{
public:
    /**
     * Compare two trustlevels
     * @return 0 if a == b, -1 if a <= b, 1 if a > b
     */
    static int compare(infrastructure::DacTypes::TrustLevel::Enum a,
                       infrastructure::DacTypes::TrustLevel::Enum b);

private:
    static int trustLevelOrdinal(infrastructure::DacTypes::TrustLevel::Enum trustLevel);
};

} // namespace joynr
#endif // TRUSTLEVELCOMPARATOR_H
