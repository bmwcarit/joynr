/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#include <QString>
#include <QByteArray>

#include "joynr/JoynrExport.h"

namespace joynr
{

class JoynrMessage;

class JOYNR_EXPORT IPlatformSecurityManager
{
public:
    virtual ~IPlatformSecurityManager()
    {
    }

    /**
     * \return the platform user ID of the running process.
     */
    virtual QString getCurrentProcessUserId() = 0;

    /**
     * \param message
     * \return signed JoynrMessage
     */
    virtual JoynrMessage sign(JoynrMessage message) = 0;

    /**
     * \param message
     * \return if message is valid returns true
     */
    virtual bool validate(const JoynrMessage& message) const = 0;

    /**
     * \param message
     * \return encrypted JoynrMessage
     */
    virtual QByteArray encrypt(const QByteArray& unencryptedBytes) = 0;

    /**
     * \param message
     * \return decrypted JoynrMessage
     */
    virtual QByteArray decrypt(const QByteArray& encryptedBytes) = 0;
};

} // namespace joynr
#endif // IPLATFORMSECURITYMANAGER_H_
