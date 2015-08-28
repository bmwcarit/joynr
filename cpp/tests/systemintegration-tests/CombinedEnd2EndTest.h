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

#ifndef COMBINEDEND2ENDTEST_H
#define COMBINEDEND2ENDTEST_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

#include <QtConcurrent/QtConcurrent>
#include <QFile>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/types/QtProviderQos.h"
#include "joynr/types/QtCapabilityInformation.h"
#include "joynr/MessagingSettings.h"
#include "joynr/joynrlogging.h"

 /*
  * This test creates two Runtimes and will test communication
  * between the two Runtimes via HttpReceiver
  *
  */
class CombinedEnd2EndTest : public testing::Test {
public:
    CombinedEnd2EndTest();
    ~CombinedEnd2EndTest();

    void SetUp();
    void TearDown();

    joynr::types::QtProviderQos qRegisterMetaTypeQos; //this is necessary to force a qRegisterMetaType<types::QtProviderQos>(); during setup
    joynr::types::QtCapabilityInformation qRegisterMetaTypeCi; //this is necessary to force a qRegisterMetaType<types::QtProviderQos>(); during setup
    joynr::JoynrRuntime* runtime1;
    joynr::JoynrRuntime* runtime2;
    std::string registeredSubscriptionId;
    QSettings settings1;
    QSettings settings2;
    joynr::MessagingSettings messagingSettings1;
    joynr::MessagingSettings messagingSettings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    QSemaphore semaphore;

    static joynr::joynr_logging::Logger* logger;

private:
    DISALLOW_COPY_AND_ASSIGN(CombinedEnd2EndTest);

};

#endif // COMBINEDEND2ENDTEST_H

