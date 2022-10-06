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
#ifndef DUMMYPLATFORMSECURITYMANAGER_H_
#define DUMMYPLATFORMSECURITYMANAGER_H_

#include <string>

#include "joynr/IPlatformSecurityManager.h"
#include "joynr/Logger.h"

namespace joynr
{

class ImmutableMessage;
class MutableMessage;

class DummyPlatformSecurityManager : public IPlatformSecurityManager
{
public:
    DummyPlatformSecurityManager() = default;

    ~DummyPlatformSecurityManager() override = default;

    std::string getCurrentProcessUserId() const override;
    void sign(MutableMessage& message) override;
    bool validate(const ImmutableMessage& message) const override;
    std::string encrypt(const std::string& unencryptedBytes) override;
    std::string decrypt(const std::string& encryptedBytes) override;

private:
    ADD_LOGGER(DummyPlatformSecurityManager)
};

} // namespace joynr
#endif // DUMMYPLATFORMSECURITYMANAGER_H_
