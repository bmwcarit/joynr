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
#ifndef NOCOMPATIBLEPROVIDERFOUNDEXCEPTION_H
#define NOCOMPATIBLEPROVIDERFOUNDEXCEPTION_H

#include <string>
#include <unordered_set>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/Version.h"

namespace joynr
{

namespace exceptions
{

/**
 * @brief Joynr exception to report noncompatible versions.
 */
class JOYNR_EXPORT NoCompatibleProviderFoundException : public DiscoveryException
{
public:
    /**
     * @brief Constructor for a NoCompatibleProviderFoundException without detail message.
     */
    NoCompatibleProviderFoundException() noexcept = default;
    /**
     * @brief Constructor for a NoCompatibleProviderFoundException with
     * a set of the incompatible versions.
     *
     * @param discoveredIncompatibleVersion the versions of the discovered providers with
     *incompatible version.
     */
    explicit NoCompatibleProviderFoundException(const std::unordered_set<joynr::types::Version>&
                                                        discoveredIncompatibleVersion) noexcept;
    const std::string& getTypeName() const override;
    /**
     * @return The versions of the discovered providers with incompatible version.
     */
    const std::unordered_set<joynr::types::Version>& getDiscoveredIncompatibleVersions() const;
    NoCompatibleProviderFoundException* clone() const override;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string TYPE_NAME;

private:
    std::unordered_set<joynr::types::Version> _discoveredIncompatibleVersions;
};

} // namespace exceptions

} // namespace joynr
#endif // NOCOMPATIBLEPROVIDERFOUNDEXCEPTION_H
