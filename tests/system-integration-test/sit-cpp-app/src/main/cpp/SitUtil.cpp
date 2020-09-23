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

#include "SitUtil.h"

#include <string>
#include <memory>

#include <joynr/JoynrRuntime.h>

#include <joynr/tests/DummyKeyChainParameters.h>
#include <joynr/tests/DummyKeychainImpl.h>

namespace joynr
{
namespace sitUtil
{
std::shared_ptr<joynr::JoynrRuntime> createRuntime(const std::string& pathToSettings,
                                                   const std::string& sslCertFilename,
                                                   const std::string& sslPrivateKeyFilename,
                                                   const std::string& sslCaCertFilename)
{
    const std::string pathToMessagingSettingsDefault("");
    std::shared_ptr<IKeychain> keychain;
    std::string privateKeyPwd;
    const joynr::tests::DummyKeyChainParameters inputKeyChainParams{
            sslCertFilename, sslPrivateKeyFilename, sslCaCertFilename, privateKeyPwd};

    if (!inputKeyChainParams.allEmpty()) {
        keychain = joynr::tests::DummyKeychainImpl::createFromPEMFiles(inputKeyChainParams);
    } else if (inputKeyChainParams.atLeastOneIsDefined()) {
        // do not throw if nothing is defined to allow initialization without keychain
        throw std::invalid_argument("All three ssl options must be provided (cert, key, ca-cert)");
    }

    // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
    // implementation.
    std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
            [](const joynr::exceptions::JoynrRuntimeException& exception) {
        std::cout << "Unexpected joynr runtime error occured: " << exception.getMessage()
                  << std::endl;
    };

    std::shared_ptr<JoynrRuntime> runtime;
    try {
        runtime = JoynrRuntime::createRuntime(
                pathToSettings, onFatalRuntimeError, pathToMessagingSettingsDefault, keychain);
    } catch (...) {
        runtime = nullptr;
    }
    return runtime;
}
}
} // namespace joynr
