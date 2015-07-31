/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/QtSubscriptionQos.h"
#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/QtOnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/QtPeriodicSubscriptionQos.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"

using namespace joynr;

TEST(SubscriptionQosTest, createQt_SubscriptionQos) {
    SubscriptionQos origin;
    QtSubscriptionQos* test = QtSubscriptionQos::createQt(origin);
    EXPECT_TRUE(dynamic_cast<QtSubscriptionQos*>(test) != NULL);
    delete test;
}

TEST(SubscriptionQosTest, createQt_OnChangeSubscriptionQos) {
    OnChangeSubscriptionQos origin;
    QtSubscriptionQos* test = QtSubscriptionQos::createQt(origin);
    EXPECT_TRUE(dynamic_cast<QtOnChangeSubscriptionQos*>(test) != NULL);
    delete test;

    test = QtSubscriptionQos::createQt(dynamic_cast<const SubscriptionQos&>(origin));
    EXPECT_TRUE(dynamic_cast<QtOnChangeSubscriptionQos*>(test) != NULL);
    delete test;
}

TEST(SubscriptionQosTest, createQt_OnChangeWithKeepAliveSubscriptionQos) {
    OnChangeWithKeepAliveSubscriptionQos origin;
    QtSubscriptionQos* test = QtSubscriptionQos::createQt(origin);
    EXPECT_TRUE(dynamic_cast<QtOnChangeWithKeepAliveSubscriptionQos*>(test) != NULL);
    delete test;
}

TEST(SubscriptionQosTest, createQt_PeriodicSubscriptionQos) {
    PeriodicSubscriptionQos origin;
    QtSubscriptionQos* test = QtSubscriptionQos::createQt(origin);
    EXPECT_TRUE(dynamic_cast<QtPeriodicSubscriptionQos*>(test) != NULL);
    delete test;
}
