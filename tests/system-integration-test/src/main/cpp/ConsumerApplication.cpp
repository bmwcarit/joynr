/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <limits>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "joynr/DiscoveryQos.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/JsonSerializer.h"
#include "joynr/Logger.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/test/SystemIntegrationTestProxy.h"

using namespace joynr;

//------- Main entry point -------------------------------------------------------

int main(int argc, char* argv[])
{
    // Get a logger
    joynr::Logger logger("ConsumerApplication");

    // Check the usage
    std::string programName(argv[0]);
    if (argc != 2) {
        JOYNR_LOG_ERROR(logger, "USAGE: {} <provider-domain> <addend-A> <addend-B>", programName);
        return -1;
    }

    // Get the provider domain
    std::string providerDomain(argv[1]);
    JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);

    JoynrRuntime* runtime = JoynrRuntime::createRuntime("");

    // Create proxy builder
    ProxyBuilder<test::SystemIntegrationTestProxy>* proxyBuilder =
            runtime->createProxyBuilder<test::SystemIntegrationTestProxy>(providerDomain);

    // Find the provider with the highest priority set in ProviderQos
    DiscoveryQos discoveryQos;
    // As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
    // is triggered. If the discovery process does not find matching providers within the
    // arbitration timeout duration it will be terminated and you will get an arbitration exception.
    discoveryQos.setDiscoveryTimeoutMs(40000);
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
    test::SystemIntegrationTestProxy* proxy = proxyBuilder->setMessagingQos(MessagingQos())
                                                      ->setDiscoveryQos(discoveryQos)
                                                      ->setCached(false)
                                                      ->build();

    bool success = true;

    const std::int32_t addendA = std::rand() % 1000;
    const std::int32_t addendB = std::rand() % 1000;
    std::int32_t sum = 0;

    try {
        proxy->add(sum, addendA, addendB);
        success = (sum == addendA + addendB);
    } catch (exceptions::JoynrException& e) {
        // exception sink
        success = false;
    }

    delete proxy;
    delete proxyBuilder;
    delete runtime;

    return success ? 0 : -1;
}
