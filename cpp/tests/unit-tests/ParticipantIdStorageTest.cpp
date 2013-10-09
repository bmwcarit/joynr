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

#include "gtest/gtest.h"
#include "joynr/ParticipantIdStorage.h"

#include <QFile>
using namespace joynr;

static const QString storageFile("participantIdStorageTest.settings");

class ParticipantIdStorageTest : public ::testing::Test {
public:
    ParticipantIdStorageTest(){}

    void SetUp()
    {
        QFile::remove(storageFile);
    }

};


// Test that the default participant id is used when no provider exists"
TEST_F(ParticipantIdStorageTest, defaultProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);

    QString participantId = store.getProviderParticipantId("interface.mytest",
                                                           "myauthtoken",
                                                           "defaultParticipantId");
    ASSERT_EQ(QString("defaultParticipantId"), participantId);
}

// Test that a participant id is created when no provider exists and
// no default value is given
TEST_F(ParticipantIdStorageTest, newProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);
    QString participantId = store.getProviderParticipantId("interface.mytest",
                                                           "myauthtoken",
                                                           QString());
    // Check that the id is long enough to be a UUID
    ASSERT_TRUE(participantId.size() > 32);
}

// Test that a persisted participant id is used
TEST_F(ParticipantIdStorageTest, persistedProviderParticipantId)
{
    ParticipantIdStorage *store = new ParticipantIdStorage(storageFile);

    QString participantId = store->getProviderParticipantId("interface.mytest",
                                                            "myauthtoken",
                                                            "persistMe");
    ASSERT_EQ(QString("persistMe"), participantId);

    // Delete the current store and create a new one
    delete store;
    store = new ParticipantIdStorage(storageFile);

    // Check that the setting was persisted
    participantId = store->getProviderParticipantId("interface.mytest",
                                                    "myauthtoken",
                                                    QString());

    ASSERT_EQ(QString("persistMe"), participantId);
    delete store;
}



