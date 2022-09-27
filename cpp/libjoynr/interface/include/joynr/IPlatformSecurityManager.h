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
#ifndef IPLATFORMSECURITYMANAGER_H_
#define IPLATFORMSECURITYMANAGER_H_

#include <string>

#include "joynr/JoynrExport.h"

namespace joynr
{

class MutableMessage;
class ImmutableMessage;

class JOYNR_EXPORT IPlatformSecurityManager
{
public:
    virtual ~IPlatformSecurityManager() = default;

    /**
     * @return the platform user ID of the running process.
     */
    virtual std::string getCurrentProcessUserId() const = 0;

    /**
     * @param message MutableMessage
     */
    virtual void sign(MutableMessage& message) = 0;

    /**
     * @param message
     * @return if message is valid returns true
     */
    virtual bool validate(const ImmutableMessage& message) const = 0;

    /**
     * @param message
     * @return encrypted JoynrMessage
     */
    virtual std::string encrypt(const std::string& unencryptedBytes) = 0;

    /**
     * @param message
     * @return decrypted JoynrMessage
     */
    virtual std::string decrypt(const std::string& encryptedBytes) = 0;
};

} // namespace joynr
#endif // IPLATFORMSECURITYMANAGER_H_
