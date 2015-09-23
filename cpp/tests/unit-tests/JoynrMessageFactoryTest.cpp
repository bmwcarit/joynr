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
#include "gtest/gtest.h"
#include "joynr/JoynrMessageFactory.h"
#include "utils/TestQString.h"
#include "utils/QThreadSleep.h"
#include "joynr/Request.h"
#include "joynr/DeclareMetatypeUtil.h"
#include "common/rpc/RpcMetaTypes.h"
#include "joynr/Reply.h"
#include "joynr/MessagingQos.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/joynrlogging.h"

using namespace joynr;

class JoynrMessageFactoryTest : public ::testing::Test {
public:
    JoynrMessageFactoryTest()
        : logger(joynr_logging::Logging::getInstance()->getLogger(QString("TEST"), QString("JoynrMessageFactoryTest"))),
          messageFactory(),
          senderID(),
          receiverID(),
          requestReplyID(),
          qos(),
          request(),
          reply(),
          subscriptionPublication()
    {
        registerRpcMetaTypes();
    }

    void SetUp(){
        senderID = QString("senderId");
        receiverID = QString("receiverID");
        requestReplyID = QString("requestReplyID");
        qos = MessagingQos(456000);
        request.setMethodName("methodName");
        request.setRequestReplyId(requestReplyID);
        ;
        request.addParam(42, "java.lang.Integer");
        request.addParam("value", "java.lang.String");
        reply.setRequestReplyId(requestReplyID);
        QList<QVariant> response;
        response.append(QVariant("response"));
        reply.setResponse(response);

        QString subscriptionId("subscriptionTestId");
        subscriptionPublication.setSubscriptionId(subscriptionId);
        response.clear();
        response.append("publication");
        subscriptionPublication.setResponse(response);
    }
    void TearDown(){

    }
    void checkHeaderCreatorFromTo(const JoynrMessage& joynrMessage){
        EXPECT_TRUE(joynrMessage.containsHeaderCreatorUserId());
        EXPECT_QSTREQ(senderID, joynrMessage.getHeaderFrom());
        EXPECT_QSTREQ(receiverID, joynrMessage.getHeaderTo());
    }

    void checkRequest(const JoynrMessage& joynrMessage){
        //TODO create expected string from params and methodName
        QString expectedPayload = QString(
                    "{\"_typeName\":\"joynr.Request\","
                    "\"methodName\":\"methodName\","
                    "\"paramDatatypes\":[\"java.lang.Integer\",\"java.lang.String\"],"
                    "\"params\":[42,\"value\"],"
                    "\"requestReplyId\":\"%1\"}"
        );
        expectedPayload = expectedPayload.arg(request.getRequestReplyId());
        EXPECT_EQ(expectedPayload, QString(joynrMessage.getPayload()));
    }

    void checkReply(const JoynrMessage& joynrMessage){
        QString expectedPayload = QString(
                    "{\"_typeName\":\"joynr.Reply\","
                    "\"requestReplyId\":\"%1\","
                    "\"response\":[\"response\"]}"
        );
        expectedPayload = expectedPayload.arg(reply.getRequestReplyId());
        EXPECT_EQ(expectedPayload, QString(joynrMessage.getPayload()));
    }

    void checkSubscriptionPublication(const JoynrMessage& joynrMessage){
        QString expectedPayload = QString(
                    "{\"_typeName\":\"joynr.SubscriptionPublication\","
                    "\"response\":[\"publication\"],"
                    "\"subscriptionId\":\"%1\"}"
        );
        expectedPayload = expectedPayload.arg(subscriptionPublication.getSubscriptionId());
        EXPECT_EQ(expectedPayload, QString(joynrMessage.getPayload()));
    }

protected:
    joynr_logging::Logger* logger;
    JoynrMessageFactory messageFactory;
    QString senderID;
    QString receiverID;
    QString requestReplyID;
    MessagingQos qos;
    Request request;
    Reply reply;
    SubscriptionPublication subscriptionPublication;
};

TEST_F(JoynrMessageFactoryTest, createRequest_withContentType) {
    JoynrMessage joynrMessage = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request
    );
    checkHeaderCreatorFromTo(joynrMessage);
}

TEST_F(JoynrMessageFactoryTest, createRequest){
    JoynrMessage joynrMessage = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request
    );
    //warning if prepareRequest needs to long this assert will fail as it compares absolute timestamps
    QDateTime expectedExpiryDate = QDateTime::currentDateTime().addMSecs(qos.getTtl());
    QDateTime expiryDate = joynrMessage.getHeaderExpiryDate();
    EXPECT_NEAR(expectedExpiryDate.toMSecsSinceEpoch(), expiryDate.toMSecsSinceEpoch(), 100.);
    LOG_DEBUG(logger,
              QString("expiryDate: %1 [%2]")
              .arg(expiryDate.toString())
              .arg(expiryDate.toMSecsSinceEpoch()));
    LOG_DEBUG(logger,
              QString("expectedExpiryDate: %1 [%2]")
              .arg(expectedExpiryDate.toString())
              .arg(expectedExpiryDate.toMSecsSinceEpoch()));

    checkHeaderCreatorFromTo(joynrMessage);
    checkRequest(joynrMessage);
    EXPECT_QSTREQ(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createReply){
    JoynrMessage joynrMessage = messageFactory.createReply(
                senderID,
                receiverID,
                qos,
                reply
    );
    checkHeaderCreatorFromTo(joynrMessage);
    checkReply(joynrMessage);
    EXPECT_QSTREQ(JoynrMessage::VALUE_MESSAGE_TYPE_REPLY, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createOneWay){
    JoynrMessage joynrMessage = messageFactory.createOneWay(
                senderID,
                receiverID,
                qos,
                reply
    );
    checkHeaderCreatorFromTo(joynrMessage);
    checkReply(joynrMessage);
    EXPECT_QSTREQ(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY, joynrMessage.getType());
}

//TEST_F(JoynrMessageFactoryTest, createSubscriptionReply){
//    QString subscriptionId("subscriptionTestId");
//    JoynrMessage joynrMessage = JoynrMessageFactory::prepareSubscriptionReply(senderID, receiverID, payload, subscriptionId);
//    checkHeaderCreatorFromTo(joynrMessage);
//    checkPayload(joynrMessage);
//    EXPECT_QSTREQ(subscriptionId, joynrMessage.getHeader<QString>(JoynrMessage::HEADER_NAME_SUBSCRIPTION_ID));
//    EXPECT_QSTREQ(JoynrMessage::MESSAGE_TYPE_SUBSCRIPTION_REPLY, joynrMessage.getType());
//}

TEST_F(JoynrMessageFactoryTest, createPublication){
    JoynrMessage joynrMessage = messageFactory.createSubscriptionPublication(
                senderID,
                receiverID,
                qos,
                subscriptionPublication
    );
    checkHeaderCreatorFromTo(joynrMessage);
    checkSubscriptionPublication(joynrMessage);
    EXPECT_QSTREQ(JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createSubscriptionRequest){
    auto subscriptionQos = std::shared_ptr<QtSubscriptionQos>(new QtOnChangeSubscriptionQos());
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(QString("subscriptionId"));
    subscriptionRequest.setSubscribeToName(QString("attributeName"));
    subscriptionRequest.setQos(subscriptionQos);
    JoynrMessage joynrMessage = messageFactory.createSubscriptionRequest(
                senderID,
                receiverID,
                qos,
                subscriptionRequest
    );
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_QSTREQ(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, createSubscriptionStop){
    QString subscriptionId("TEST-SubscriptionId");
    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionId);
    JoynrMessage joynrMessage = messageFactory.createSubscriptionStop(
                senderID,
                receiverID,
                qos,
                subscriptionStop
    );
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_QSTREQ(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP, joynrMessage.getType());
}

TEST_F(JoynrMessageFactoryTest, testRequestContentType){
    Request request;
    QVariantList params;
    params.append("test");
    request.setMethodName(QString("methodName"));
    request.setParams(params);

    JoynrMessage message = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request
    );
    EXPECT_QSTREQ(JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON, message.getHeaderContentType());
}
