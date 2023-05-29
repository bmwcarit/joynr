/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

#include "LocalCapabilitiesDirectoryAbstract.h"

const std::vector<std::string> AbstractLocalCapabilitiesDirectoryTest::_KNOWN_GBIDS{
        "testGbid1", "testGbid2", "testGbid3"};
const std::string AbstractLocalCapabilitiesDirectoryTest::_INTERFACE_1_NAME("myInterfaceA");
const std::string AbstractLocalCapabilitiesDirectoryTest::_INTERFACE_2_NAME("myInterfaceB");
const std::string AbstractLocalCapabilitiesDirectoryTest::_INTERFACE_3_NAME("myInterfaceC");
const std::string AbstractLocalCapabilitiesDirectoryTest::_DOMAIN_1_NAME("domainA");
const std::string AbstractLocalCapabilitiesDirectoryTest::_DOMAIN_2_NAME("domainB");
const std::string AbstractLocalCapabilitiesDirectoryTest::_DOMAIN_3_NAME("domainB");
const std::string AbstractLocalCapabilitiesDirectoryTest::_LOCAL_ADDRESS(serializer::serializeToJson(
        system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "localTopic")));
const std::vector<std::string> AbstractLocalCapabilitiesDirectoryTest::_EXTERNAL_ADDRESSES_VECTOR{
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[1], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[2], "externalTopic"))};
const std::int64_t AbstractLocalCapabilitiesDirectoryTest::_LASTSEEN_MS(1000);
const std::int64_t AbstractLocalCapabilitiesDirectoryTest::_EXPIRYDATE_MS(10000);
const std::string AbstractLocalCapabilitiesDirectoryTest::_PUBLIC_KEY_ID("publicKeyId");
const int AbstractLocalCapabilitiesDirectoryTest::_TIMEOUT(2000);