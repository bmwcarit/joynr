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

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <stdexcept>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include "joynr/DiscoveryQos.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/test/SystemIntegrationTestProxy.h"
#include "SitUtil.h"
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

using namespace joynr;

//------- Main entry point -------------------------------------------------------

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYSC", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    joynr::Logger logger("ConsumerApplication");

    namespace po = boost::program_options;

    po::positional_options_description positionalCmdLineOptions;
    positionalCmdLineOptions.add("domain", 1);

    std::string providerDomain;
    std::string pathToSettings;
    std::string sslCertFilename;
    std::string sslPrivateKeyFilename;
    std::string sslCaCertFilename;
    std::uint64_t runs = 0;

    po::options_description cmdLineOptions("Available options");
    cmdLineOptions.add_options()(
            "domain,d", po::value(&providerDomain)->required(), "joynr domain to be used")(
            "pathtosettings,p",
            po::value(&pathToSettings),
            "Absolute path to a non-default setting file.")(
            "ssl-cert-pem",
            po::value(&sslCertFilename),
            "Absolute path to public certificate for this application.")(
            "ssl-privatekey-pem",
            po::value(&sslPrivateKeyFilename),
            "Absolute path to private key for this application.")(
            "ssl-ca-cert-pem",
            po::value(&sslCaCertFilename),
            "Absolute path to certificate of CA.")("runs,r",
                                                   po::value(&runs)->default_value(1),
                                                   "Specify number of RPC calls to perform.")(
            "help,h", "Print help message");

    try {
        po::variables_map variablesMap;
        po::store(po::command_line_parser(argc, argv)
                          .options(cmdLineOptions)
                          .positional(positionalCmdLineOptions)
                          .run(),
                  variablesMap);

        if (variablesMap.count("help")) {
            std::cout << cmdLineOptions << std::endl;
            return EXIT_SUCCESS;
        }

        po::notify(variablesMap);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << cmdLineOptions << std::endl;
        return -1;
    }

    JOYNR_LOG_INFO(logger, "Create proxy for domain {}", providerDomain);

    if (pathToSettings.empty()) {
        boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
        std::string appDirectory =
                boost::filesystem::system_complete(appFilename).parent_path().string();
        pathToSettings = appDirectory + "/resources/systemintegrationtest-consumer.settings";
    }

    const std::string pathToMessagingSettingsDefault("");
    std::shared_ptr<IKeychain> keychain;

    try {
        keychain = tryLoadKeychainFromCmdLineArgs(
                sslCertFilename, sslPrivateKeyFilename, sslCaCertFilename);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_FATAL(logger, e.what());
        return -1;
    }

    std::shared_ptr<JoynrRuntime> runtime =
            JoynrRuntime::createRuntime(pathToSettings, pathToMessagingSettingsDefault, keychain);

    std::shared_ptr<ProxyBuilder<test::SystemIntegrationTestProxy>> proxyBuilder =
            runtime->createProxyBuilder<test::SystemIntegrationTestProxy>(providerDomain);

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Mins
    discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

    std::shared_ptr<test::SystemIntegrationTestProxy> proxy(
            proxyBuilder->setMessagingQos(MessagingQos())->setDiscoveryQos(discoveryQos)->build());

    bool success = true;

    const std::int32_t addendA = std::rand() % 1000;
    const std::int32_t addendB = std::rand() % 1000;
    std::int32_t sum = 0;

    try {
        while (runs-- > 0) {
            proxy->add(sum, addendA, addendB);
            success = (sum == addendA + addendB);
            JOYNR_LOG_INFO(logger,
                           "SIT RESULT success: C++ consumer -> {} ({} + {} = {})",
                           providerDomain,
                           addendA,
                           addendB,
                           sum);
        }
    } catch (const exceptions::JoynrException& e) {
        // exception sink
        success = false;
        JOYNR_LOG_ERROR(
                logger, "SIT RESULT error: \"{}\" : C++ consumer -> {}", e.what(), providerDomain);
    }

    return success ? 0 : 1;
}
