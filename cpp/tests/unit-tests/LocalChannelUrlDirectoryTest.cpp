/*
 * #%L
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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/LocalChannelUrlDirectory.h"
#include "utils/QThreadSleep.h"
#include "tests/utils/MockObjects.h"
#include "joynr/Future.h"

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::SetArgReferee;
using ::testing::Return;
using ::testing::Invoke;


using namespace joynr;

// global function used for calls to the MockChannelUrlSelectorProxy
void localChannelUrlDirectoryTestPseudoGetChannelUrls(QSharedPointer<Future<types::ChannelUrlInformation> > future , QString channelId) {
    types::ChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << "firstUrl" << "secondUrl" << "thirdUrl";
    urlInformation.setUrls(urls);
    future->onSuccess(RequestStatus(RequestStatusCode::OK), urlInformation);
}


TEST(LocalChannelUrlDirectoryTest, getChannelUrlsUsesInternalProxy) {
    QSharedPointer<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy =
            QSharedPointer<MockChannelUrlDirectoryProxy>(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, getUrlsForChannel(
                    A<QSharedPointer<Future<types::ChannelUrlInformation> > >(),
                    A<QString>()))
            .WillOnce(Invoke(localChannelUrlDirectoryTestPseudoGetChannelUrls));

    LocalChannelUrlDirectory localDirectory(mockChannelUrlDirectoryProxy, "ChannelDirectorypseudoUrl");
    QSharedPointer<Future<types::ChannelUrlInformation> > futureUrls = QSharedPointer<Future<types::ChannelUrlInformation> >(
                new Future<types::ChannelUrlInformation>());
    localDirectory.getUrlsForChannel(futureUrls,"pseudoChannelID", 20000);

    EXPECT_EQ(true, futureUrls->getStatus().successful());

    types::ChannelUrlInformation channelInf = futureUrls->getValue();
    types::ChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << "firstUrl" << "secondUrl" << "thirdUrl";
    urlInformation.setUrls(urls);

    EXPECT_EQ(urlInformation,channelInf);
}

TEST(LocalChannelUrlDirectoryTest, registerChannelUrls) {
    QSharedPointer<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy =
            QSharedPointer<MockChannelUrlDirectoryProxy>(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, registerChannelUrls(
                    A<QSharedPointer<Future<void > > >(),
                    A<QString>(),
                    _)).Times(1);

    LocalChannelUrlDirectory localDirectory(mockChannelUrlDirectoryProxy, "ChannelDirectorypseudoUrl");

    types::ChannelUrlInformation urlInformation;
    QSharedPointer<Future<void> > future(new Future<void>());
    localDirectory.registerChannelUrls(future, "myChannelId", urlInformation );
}

TEST(LocalChannelUrlDirectoryTest, unregisterChannelUrls) {
    QSharedPointer<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy =
            QSharedPointer<MockChannelUrlDirectoryProxy>(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, unregisterChannelUrls(
                    A<QSharedPointer<Future<void > > >(),
                    _)).Times(1);

    LocalChannelUrlDirectory localDirectory(mockChannelUrlDirectoryProxy, "ChannelDirectorypseudoUrl");

    types::ChannelUrlInformation urlInformation;
    QSharedPointer<Future<void> > future(new Future<void>());
    localDirectory.unregisterChannelUrls(future,"pseudoChannelId");
}

















