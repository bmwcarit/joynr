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

#include <future>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <joynr/JoynrRuntime.h>
#include <joynr/Logger.h>

#ifdef DUMMY_KEYCHAIN
#include <joynr/tests/DummyKeyChainParameters.h>
#include <joynr/tests/DummyKeychainImpl.h>
#elif REAL_KEYCHAIN
#include <libcertmanager/private.h>
#include <libcertmanager/public.h>
#endif

namespace joynr
{
namespace sitUtil
{

static joynr::Logger logger("SitUtil");

#ifdef DUMMY_KEYCHAIN
/**
 * This will only create a dummy keychain object. For testing use only!
 */
std::shared_ptr<IKeychain> createKeychain(const std::string& sslCertFilename,
                                          const std::string& sslPrivateKeyFilename,
                                          const std::string& sslCaCertFilename)
{
    JOYNR_LOG_INFO(logger, "Creating dummy keychain");
    std::shared_ptr<IKeychain> keychain;
    std::string privateKeyPwd;
    const joynr::tests::DummyKeyChainParameters inputKeyChainParams{
            sslCertFilename, sslPrivateKeyFilename, sslCaCertFilename, privateKeyPwd};

    if (!inputKeyChainParams.allEmpty()) {
        return joynr::tests::DummyKeychainImpl::createFromPEMFiles(inputKeyChainParams);
    } else if (inputKeyChainParams.atLeastOneIsDefined()) {
        // do not throw if nothing is defined to allow initialization without keychain
        throw std::invalid_argument("All three ssl options must be provided (cert, key, ca-cert)");
    }

    return nullptr;
}
#elif REAL_KEYCHAIN
/**
 * Create a keychain object using the libcertmanager.
 */
std::shared_ptr<IKeychain> createKeychain(const std::string&,
                                          const std::string&,
                                          const std::string&)
{
    JOYNR_LOG_INFO(logger, "Creating real keychain");
    char keychaindir[] = "joynr-test-XXXXXX";
    if (!mkdtemp(keychaindir)) {
        std::stringstream msg;
        msg << "Could not create temporary directory: " << strerror(errno) << "(" << errno << ")";
        throw std::runtime_error(msg.str());
    }

    certmgr::setupKeychain("joynrbattest", keychaindir);
    std::promise<std::unique_ptr<joynr::certmgr::Keychain>> promise;
    auto future = promise.get_future();

    certmgr::useKeychain(keychaindir, [&promise](auto state, auto keychain) {
        if (state != certmgr::KeychainState::READY) {
            JOYNR_LOG_DEBUG(logger, "Received state: {}", state);
            return;
        }

        JOYNR_LOG_INFO(logger, "Received valid key chain from libcertmanager");
        promise.set_value(std::move(keychain));
    });

    return future.get();
}
#else
/**
 * No keychain is used at all
 */
std::shared_ptr<IKeychain> createKeychain(const std::string&,
                                          const std::string&,
                                          const std::string&)
{
    JOYNR_LOG_INFO(logger, "Creating null keychain");
    return nullptr;
}
#endif

std::shared_ptr<joynr::JoynrRuntime> createRuntime(const std::string& pathToSettings,
                                                   const std::string& sslCertFilename,
                                                   const std::string& sslPrivateKeyFilename,
                                                   const std::string& sslCaCertFilename)
{
    const std::string pathToMessagingSettingsDefault("");

    auto keychain = createKeychain(sslCertFilename, sslPrivateKeyFilename, sslCaCertFilename);

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
} // namespace sitUtil
} // namespace joynr
