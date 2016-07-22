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
#include "IltAbstractConsumerTest.h"

INIT_LOGGER(IltAbstractConsumerTest);

joynr::interlanguagetest::TestInterfaceProxy* IltAbstractConsumerTest::testInterfaceProxy = nullptr;
ProxyBuilder<interlanguagetest::TestInterfaceProxy>* IltAbstractConsumerTest::proxyBuilder =
        nullptr;
JoynrRuntime* IltAbstractConsumerTest::runtime = nullptr;
std::string IltAbstractConsumerTest::providerDomain = "joynr-inter-language-test-domain";
std::string IltAbstractConsumerTest::programName;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    IltAbstractConsumerTest::setProgramName(std::string(argv[0]));

    return RUN_ALL_TESTS();
}
