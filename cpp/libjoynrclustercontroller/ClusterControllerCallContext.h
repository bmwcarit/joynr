/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#ifndef CLUSTERCONTROLLERCALLCONTEXT_H
#define CLUSTERCONTROLLERCALLCONTEXT_H

#include "joynr/Logger.h"

namespace joynr
{
class ClusterControllerCallContext
{
public:
    ClusterControllerCallContext();
    ClusterControllerCallContext(ClusterControllerCallContext&&) = default;
    ClusterControllerCallContext& operator=(ClusterControllerCallContext&&) = default;
    ClusterControllerCallContext(const ClusterControllerCallContext&) = default;
    ClusterControllerCallContext& operator=(const ClusterControllerCallContext&) = default;

    void setIsValid(bool isValid);
    bool getIsValid() const;

    void setIsInternalProviderRegistration(bool isInternalProviderRegistration);
    bool getIsInternalProviderRegistration() const;

    void invalidate();

private:
    bool _isValid;
    bool _isInternalProviderRegistration;

    ADD_LOGGER(ClusterControllerCallContext)
};
} // namespace joynr

#endif // CLUSTERCONTROLLERCALLCONTEXT_H
