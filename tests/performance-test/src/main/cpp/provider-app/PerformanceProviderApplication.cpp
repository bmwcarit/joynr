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

#include <cstdlib>
#include <string>
#include <chrono>
#include <thread>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include <joynr/Logger.h>
#include <joynr/JoynrRuntime.h>
#include <joynr/Settings.h>
#include <joynr/WebSocketSettings.h>

#include <joynr/tests/DummyKeychainImpl.h>
#include <joynr/tests/DummyKeyChainParameters.h>

#include "../provider/PerformanceTestEchoProvider.h"

#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

using namespace joynr;

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYPP", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    Logger logger("PerformanceTestProviderApplication");

    std::string domainName;
    bool globalScope = false;
    bool useKeyChain = false;
    const std::string ccUrlForTLS("wss://localhost:4243");
    joynr::tests::DummyKeyChainParameters keyChainInputParams;

    boost::program_options::options_description optionsDescription("Available options");
    optionsDescription.add_options()("help,h", "produce help message")(
            "domain,d", boost::program_options::value(&domainName)->required(), "domain")(
            "globalscope,g", boost::program_options::value(&globalScope)->default_value(false))(
            "useKeychain",
            boost::program_options::value(&useKeyChain)->default_value(false),
            "Should KeyChain be used? Default: false. If true can run only on localhost.")(
            "root-certificate",
            boost::program_options::value(&keyChainInputParams.rootCertFileName),
            "Root certificate in PEM encoded format.")(
            "pub-certificate",
            boost::program_options::value(&keyChainInputParams.pubCertFileName),
            "Public certificate in PEM encoded format.")(
            "private-key",
            boost::program_options::value(&keyChainInputParams.privKeyFileName),
            "Private key in PEM encoded format.")(
            "private-key-pwd",
            boost::program_options::value(&keyChainInputParams.privKeyPassword)->default_value(""),
            "Passsword of private key. Default: empty string.");

    try {
        boost::program_options::variables_map optionsMap;
        boost::program_options::store(
                boost::program_options::parse_command_line(argc, argv, optionsDescription),
                optionsMap);

        if (optionsMap.count("help") > 0) {
            std::cout << optionsDescription << std::endl;
            return EXIT_SUCCESS;
        }

        boost::program_options::notify(optionsMap);

        boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
        std::string appDirectory =
                boost::filesystem::system_complete(appFilename).parent_path().string();
        auto joynrSettings = std::make_unique<joynr::Settings>(
                (appDirectory + "/resources/performancetest-provider.settings"));

        std::shared_ptr<joynr::IKeychain> keyChain;

        if (useKeyChain) {
            keyChain = joynr::tests::DummyKeychainImpl::createFromPEMFiles(keyChainInputParams);
            // also change settings file accordingly
            joynr::WebSocketSettings wsSettings(*joynrSettings);
            wsSettings.setClusterControllerMessagingUrl(ccUrlForTLS);
        }

        // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
        // implementation.
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
                [&](const joynr::exceptions::JoynrRuntimeException& exception) {
            std::cout << "Unexpected joynr runtime error occured: " << exception.getMessage()
                      << std::endl;
        };

        std::shared_ptr<joynr::JoynrRuntime> runtime(joynr::JoynrRuntime::createRuntime(
                std::move(joynrSettings), std::move(onFatalRuntimeError), std::move(keyChain)));

        std::shared_ptr<PerformanceTestEchoProvider> provider =
                std::make_shared<PerformanceTestEchoProvider>();

        // Set the provider's priority in such a way that a consumer uses the most recent provider.
        auto millisecondsSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch());

        types::ProviderQos providerQos;
        providerQos.setPriority(millisecondsSinceEpoch.count());
        providerQos.setScope(globalScope ? joynr::types::ProviderScope::GLOBAL
                                         : joynr::types::ProviderScope::LOCAL);

        runtime->registerProvider<tests::performance::EchoProvider>(
                domainName, provider, providerQos);

        JOYNR_LOG_INFO(
                logger, "********************************************************************");
        JOYNR_LOG_INFO(logger, "Provider is registered");
        JOYNR_LOG_INFO(
                logger, "********************************************************************");

        // Run the provider for a week (which should be enough for any testcase).
        std::this_thread::sleep_for(std::chrono::hours(24 * 7));

        runtime->unregisterProvider<tests::performance::EchoProvider>(domainName, provider);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
