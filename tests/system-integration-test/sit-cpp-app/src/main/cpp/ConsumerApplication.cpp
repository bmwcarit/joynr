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
    // Get a logger
    joynr::Logger logger("ConsumerApplication");

    namespace po = boost::program_options;

    po::positional_options_description positionalCmdLineOptions;
    positionalCmdLineOptions.add("domain", 1);

    std::string providerDomain;
    std::string pathToSettings;
    std::string sslCertFilename;
    std::string sslPrivateKeyFilename;
    std::string sslCaCertFilename;

    po::options_description cmdLineOptions;
    cmdLineOptions.add_options()("domain,d", po::value(&providerDomain)->required())(
            "pathtosettings,p", po::value(&pathToSettings))(
            "ssl-cert-pem", po::value(&sslCertFilename))(
            "ssl-privatekey-pem", po::value(&sslPrivateKeyFilename))(
            "ssl-ca-cert-pem", po::value(&sslCaCertFilename));

    try {
        po::variables_map variablesMap;
        po::store(po::command_line_parser(argc, argv)
                          .options(cmdLineOptions)
                          .positional(positionalCmdLineOptions)
                          .run(),
                  variablesMap);
        po::notify(variablesMap);
    } catch (const std::exception& e) {
        std::cerr << e.what();
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

    // Create proxy builder
    std::shared_ptr<ProxyBuilder<test::SystemIntegrationTestProxy>> proxyBuilder =
            runtime->createProxyBuilder<test::SystemIntegrationTestProxy>(providerDomain);

    // Find the provider with the highest priority set in ProviderQos
    DiscoveryQos discoveryQos;
    // As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
    // is triggered. If the discovery process does not find matching providers within the
    // arbitration timeout duration it will be terminated and you will get an arbitration exception.
    discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Mins
    // Provider entries in the global capabilities directory are cached locally. Discovery will
    // consider entries in this cache valid if they are younger as the max age of cached
    // providers as defined in the QoS. All valid entries will be processed by the arbitrator when
    // searching
    // for and arbitrating the "best" matching provider.
    // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
    //       directory. Therefore, not all providers registered with the global capabilities
    //       directory might be taken into account during arbitration.
    discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());
    // The discovery process outputs a list of matching providers. The arbitration strategy then
    // chooses one or more of them to be used by the proxy.
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

    // Build a proxy to communicate with the provider
    std::shared_ptr<test::SystemIntegrationTestProxy> proxy(
            proxyBuilder->setMessagingQos(MessagingQos())->setDiscoveryQos(discoveryQos)->build());

    bool success = true;

    const std::int32_t addendA = std::rand() % 1000;
    const std::int32_t addendB = std::rand() % 1000;
    std::int32_t sum = 0;

    try {
        proxy->add(sum, addendA, addendB);
        success = (sum == addendA + addendB);
        JOYNR_LOG_INFO(logger,
                       "SIT RESULT success: C++ consumer -> {} ({} + {} = {})",
                       providerDomain,
                       addendA,
                       addendB,
                       sum);
    } catch (const exceptions::JoynrException& e) {
        // exception sink
        success = false;
        JOYNR_LOG_ERROR(
                logger, "SIT RESULT error: \"{}\" : C++ consumer -> {}", e.what(), providerDomain);
    }

    return success ? 0 : 1;
}
