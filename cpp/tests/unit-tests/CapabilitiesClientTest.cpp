/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/PrivateCopyAssign.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "cluster-controller/capabilities-client/CapabilitiesClient.h"
#include "tests/utils/MockObjects.h"

using namespace joynr;

class CapabilitiesClientTest : public ::testing::Test {
public:
    CapabilitiesClientTest()
        : capClient(new CapabilitiesClient(QString("localChannel")))
    {
    }

private:
    CapabilitiesClient* capClient;
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesClientTest);
};
