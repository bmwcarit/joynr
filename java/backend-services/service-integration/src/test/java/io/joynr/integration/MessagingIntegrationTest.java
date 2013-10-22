package io.joynr.integration;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import io.joynr.capabilities.DummyCapabilitiesDirectory;
import io.joynr.capabilities.DummyDiscoveryModule;
import io.joynr.capabilities.DummyLocalChannelUrlDirectoryClient;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.JoynrMessageFactory;
import io.joynr.dispatcher.MessagingEndpointDirectory;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.integration.util.ServersUtil;
import io.joynr.integration.util.TestMessageListener;
import io.joynr.messaging.IMessageReceivers;
import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrInjectorFactory; //import io.joynr.util.PreconfiguredEndpointDirectoryModule;
import io.joynr.runtime.PropertyLoader;
import io.joynr.util.PreconfiguredEndpointDirectoryModule;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.UUID;

import joynr.JoynrMessage;
import joynr.types.ChannelUrlInformation;

import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.name.Names;

/**
 * Tests the interaction of the dispatcher and communication manager.
 */
public class MessagingIntegrationTest {

    private static final int DEFAULT_TIMEOUT = 3000;

    private static Server server;

    private MessageSender joynrMessageSender1;
    private MessageSender joynrMessageSender2;

    private String payload1;
    private String payload2;
    private String payload3;
    private String payload4;

    private String fromParticipantId = "mySenderParticipantId";
    private String toParticipantId = "myReceiverParticipantId";

    private MessageReceiver messageReceiver1;
    private MessageReceiver messageReceiver2;
    MessagingEndpointDirectory messagingEndpointDirectory1;
    MessagingEndpointDirectory messagingEndpointDirectory2;
    private JoynrMessageFactory joynrMessagingFactory;

    private static final Logger logger = LoggerFactory.getLogger(MessagingIntegrationTest.class);

    private static final String STATIC_PERSISTENCE_FILE = "target/temp/persistence.properties";
    private LocalChannelUrlDirectoryClient localChannelUrlDirectoryClient;
    private DummyCapabilitiesDirectory localCapDir;
    private String bounceProxyUrl;

    private long relativeTtl_ms = 10000L;

    @BeforeClass
    public static void startServer() throws Exception {
        server = ServersUtil.startServers();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        server.stop();
    }

    @Before
    public void setUp() throws SecurityException {
        System.getProperties().put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                   "TEST-libjoynr-domain-" + System.currentTimeMillis());

        // use the same capabilities and channelUrl directory in all injectors
        localCapDir = new DummyCapabilitiesDirectory();
        localChannelUrlDirectoryClient = new DummyLocalChannelUrlDirectoryClient();

        String channelId1 = "1_" + UUID.randomUUID().toString().substring(0, 2);
        Injector injector1 = setupMessageEndpoint(channelId1, localChannelUrlDirectoryClient, localCapDir);
        joynrMessageSender1 = injector1.getInstance(MessageSender.class);
        messageReceiver1 = injector1.getInstance(MessageReceiver.class);
        IMessageReceivers messageReceivers = injector1.getInstance(IMessageReceivers.class);
        messageReceivers.registerMessageReceiver(messageReceiver1, channelId1);
        bounceProxyUrl = injector1.getInstance(Key.get(String.class,
                                                       Names.named(MessagingPropertyKeys.BOUNCE_PROXY_URL)));

        String channelId2 = "2_" + UUID.randomUUID().toString();
        Injector injector2 = setupMessageEndpoint(channelId2, localChannelUrlDirectoryClient, localCapDir);
        joynrMessageSender2 = injector2.getInstance(MessageSender.class);
        messageReceiver2 = injector2.getInstance(MessageReceiver.class);
        IMessageReceivers messageReceivers2 = injector2.getInstance(IMessageReceivers.class);
        messageReceivers2.registerMessageReceiver(messageReceiver2, channelId2);

        joynrMessagingFactory = injector1.getInstance(JoynrMessageFactory.class);

        payload1 = "test payload 1€";
        payload2 = "test payload 2€";
        payload3 = "this message should be received";
        payload4 = "this message should be dropped";

    }

    public Injector setupMessageEndpoint(String channelId,
                                         LocalChannelUrlDirectoryClient localChannelUrlDirectoryClient,
                                         LocalCapabilitiesDirectory localCapDir) {
        MessagingEndpointDirectory messagingEndpointDirectory = new MessagingEndpointDirectory("channelurldirectory_participantid",
                                                                                               "discoverydirectory_channelid",
                                                                                               "capabilitiesdirectory_participantid",
                                                                                               "discoverydirectory_channelid");

        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        channelUrlInformation.setUrls(Arrays.asList(getChannelUrl(channelId)));
        localChannelUrlDirectoryClient.registerChannelUrls(channelId, channelUrlInformation);

        Properties joynrConfig = PropertyLoader.loadProperties("testMessaging.properties");
        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.put(MessagingPropertyKeys.CHANNELID, channelId);
        joynrConfig.put(MessagingPropertyKeys.RECEIVERID, UUID.randomUUID().toString());
        Injector injector = new JoynrInjectorFactory(joynrConfig,
                                                     new DummyDiscoveryModule(localChannelUrlDirectoryClient,
                                                                              localCapDir),
                                                     new PreconfiguredEndpointDirectoryModule(messagingEndpointDirectory)).getInjector();

        // DummyJoynApplication dummyJoynApplication = applicationFactory.createApplication(DummyJoynApplication.class);

        localChannelUrlDirectoryClient.registerChannelUrls(channelId, channelUrlInformation);

        return injector;

    }

    @After
    public void tearDown() {
        logger.info("TEAR DOWN");
        messageReceiver1.shutdown(true);
        messageReceiver2.shutdown(true);
        joynrMessageSender1.shutdown();
        joynrMessageSender2.shutdown();

    }

    @Test
    public void sendToNonexistantChannel() throws Exception {

        TestMessageListener listener1 = new TestMessageListener(0, 1);
        messageReceiver1.registerMessageListener(listener1);
        messageReceiver1.startReceiver();

        ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(30000L);
        JoynrMessage messageA = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload1,
                                                                   expiryDate);
        try {
            joynrMessageSender1.sendMessage(UUID.randomUUID().toString(), messageA);
        } catch (JoynrMessageNotSentException e) {

        }
        listener1.assertAllErrorsReceived(DEFAULT_TIMEOUT);
    }

    @Test
    public void receiveOneWayMessagesBothDirections() throws Exception {
        ExpiryDate expiryDate;
        String channelId2 = messageReceiver2.getChannelId();

        TestMessageListener listener2 = new TestMessageListener(2);
        messageReceiver2.registerMessageListener(listener2);
        messageReceiver2.startReceiver();

        TestMessageListener listener1 = new TestMessageListener(1);
        messageReceiver1.registerMessageListener(listener1);
        messageReceiver1.startReceiver();
        Thread.sleep(50);

        // send 2 messages one way
        expiryDate = ExpiryDate.fromRelativeTtl(relativeTtl_ms);
        JoynrMessage messageA = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload1,
                                                                   expiryDate);
        JoynrMessage messageB = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload2,
                                                                   expiryDate);
        joynrMessageSender1.sendMessage(channelId2, messageA);
        joynrMessageSender1.sendMessage(channelId2, messageB);

        listener2.assertAllPayloadsReceived(DEFAULT_TIMEOUT);
        listener2.assertReceivedPayloadsContains(messageA, messageB);

        // test the other direction
        String channelId1 = messageReceiver1.getChannelId();

        Thread.sleep(50);

        expiryDate = ExpiryDate.fromRelativeTtl(relativeTtl_ms);
        JoynrMessage messageC = joynrMessagingFactory.createOneWay(toParticipantId,
                                                                   fromParticipantId,
                                                                   payload2,
                                                                   expiryDate);
        joynrMessageSender2.sendMessage(channelId1, messageC);

        listener1.assertAllPayloadsReceived(DEFAULT_TIMEOUT);
        listener1.assertReceivedPayloadsContains(messageC);
    }

    @Test
    public void testUmlautInMessagesPayload() throws Exception {
        ExpiryDate ttl_absolute_ms;
        String channelId2 = messageReceiver2.getChannelId();
        // send message one way
        TestMessageListener listener2 = new TestMessageListener(1);
        messageReceiver2.registerMessageListener(listener2);
        messageReceiver2.startReceiver();
        TestMessageListener listener1 = new TestMessageListener(0, 0);
        messageReceiver1.registerMessageListener(listener1);
        messageReceiver1.startReceiver();

        Thread.sleep(50);

        ttl_absolute_ms = ExpiryDate.fromRelativeTtl(relativeTtl_ms);
        JoynrMessage messageA = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   "Test äöü",
                                                                   ttl_absolute_ms);
        joynrMessageSender1.sendMessage(channelId2, messageA);

        listener2.assertAllPayloadsReceived(DEFAULT_TIMEOUT);
        listener2.assertReceivedPayloadsContains(messageA);

    }

    @Test
    public void ttlCausesMessageToBeDropped() throws Exception {
        String channelId2 = messageReceiver2.getChannelId();

        // send 2 messages one way, one should be dropped
        TestMessageListener listener1 = new TestMessageListener(0, 1);
        messageReceiver1.registerMessageListener(listener1);
        messageReceiver1.startReceiver();

        TestMessageListener listener2 = new TestMessageListener(1);
        messageReceiver2.registerMessageListener(listener2);
        messageReceiver2.startReceiver();

        // stops long poll
        messageReceiver2.suspend();

        JoynrMessage message1 = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload3,
                                                                   ExpiryDate.fromRelativeTtl(50000));
        JoynrMessage message2 = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload4,
                                                                   ExpiryDate.fromRelativeTtl(200));
        joynrMessageSender1.sendMessage(channelId2, message1);
        joynrMessageSender1.sendMessage(channelId2, message2);

        // wait ttl to cause a message to be discarded
        Thread.sleep(500);
        messageReceiver2.resume();
        listener2.assertAllPayloadsReceived(5000);
        listener2.assertReceivedPayloadsContains(message1);
        listener2.assertReceivedPayloadsContainsNot(message2);

    }

    @Test
    public void receiveMultipleMessagesInOneResponseAndDistributeToListener() throws Exception {

        TestMessageListener listener1 = new TestMessageListener(0, 0);
        messageReceiver1.registerMessageListener(listener1);
        messageReceiver1.startReceiver();

        TestMessageListener listener2 = new TestMessageListener(2);
        messageReceiver2.registerMessageListener(listener2);
        messageReceiver2.startReceiver();

        messageReceiver2.suspend();

        ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(5000); // ttl 5 sec
        JoynrMessage message1 = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload1,
                                                                   expiryDate);
        JoynrMessage message2 = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload2,
                                                                   expiryDate);

        String channelId2 = messageReceiver2.getChannelId();

        joynrMessageSender1.sendMessage(channelId2, message1);
        joynrMessageSender1.sendMessage(channelId2, message2);

        Thread.sleep(500);
        messageReceiver2.resume();

        listener2.assertAllPayloadsReceived(DEFAULT_TIMEOUT);
        listener2.assertReceivedPayloadsContains(message1, message2);
    }

    @Test
    public void receiveOneWayMessagesBothDirections2() throws Exception {
        ExpiryDate ttl_absolute_ms;
        String channelId2 = messageReceiver2.getChannelId();

        TestMessageListener listener2 = new TestMessageListener(2);
        messageReceiver2.registerMessageListener(listener2);
        messageReceiver2.startReceiver();

        TestMessageListener listener1 = new TestMessageListener(1);
        messageReceiver1.registerMessageListener(listener1);
        messageReceiver1.startReceiver();
        Thread.sleep(50);

        // send 2 messages one way
        ttl_absolute_ms = ExpiryDate.fromRelativeTtl(relativeTtl_ms);
        JoynrMessage messageA = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload1,
                                                                   ttl_absolute_ms);
        JoynrMessage messageB = joynrMessagingFactory.createOneWay(fromParticipantId,
                                                                   toParticipantId,
                                                                   payload2,
                                                                   ttl_absolute_ms);
        joynrMessageSender1.sendMessage(channelId2, messageA);
        joynrMessageSender1.sendMessage(channelId2, messageB);

        listener2.assertAllPayloadsReceived(DEFAULT_TIMEOUT);
        listener2.assertReceivedPayloadsContains(messageA, messageB);

        // test the other direction
        String channelId1 = messageReceiver1.getChannelId();

        Thread.sleep(50);

        ttl_absolute_ms = ExpiryDate.fromRelativeTtl(relativeTtl_ms);
        JoynrMessage messageC = joynrMessagingFactory.createOneWay(toParticipantId,
                                                                   fromParticipantId,
                                                                   payload2,
                                                                   ttl_absolute_ms);
        joynrMessageSender2.sendMessage(channelId1, messageC);

        listener1.assertAllPayloadsReceived(DEFAULT_TIMEOUT);
        listener1.assertReceivedPayloadsContains(messageC);
    }

    @Test
    public void fastSendAndReplyManyMessages() throws InterruptedException, JsonGenerationException,
                                              JoynrSendBufferFullException, JoynrMessageNotSentException, IOException {
        ExpiryDate ttl_absolute_ms;
        int nMessages = 500; // TestMessageListener listener1 = new TestMessageListener(nMessages);

        String channelId2 = messageReceiver2.getChannelId();

        TestMessageListener listener2 = new TestMessageListener(nMessages);
        messageReceiver2.registerMessageListener(listener2);
        messageReceiver2.startReceiver();
        messageReceiver1.startReceiver();
        Thread.sleep(50);

        ttl_absolute_ms = ExpiryDate.fromRelativeTtl(relativeTtl_ms);

        List<JoynrMessage> messages = new ArrayList<JoynrMessage>();
        for (int i = 0; i < nMessages; i++) {
            JoynrMessage message = joynrMessagingFactory.createOneWay(fromParticipantId, toParticipantId, payload1
                    + "message:" + i, ttl_absolute_ms);
            messages.add(message);
            joynrMessageSender1.sendMessage(channelId2, message);
        }

        // wait 5 secs plus 10 ms per message extra
        listener2.assertAllPayloadsReceived(DEFAULT_TIMEOUT + nMessages * 50);

        listener2.assertReceivedPayloadsContains(messages.toArray());

    }

    // this test is a stub. There is currently on way to check, whether a channel is really deleted. One can use
    // this test to check manually
    @Test
    @Ignore
    public void deleteChannels() throws Exception {
        logger.info("channel id of dispatcher1: " + messageReceiver1.getChannelId());
        // Thread.sleep(5 * 1000);
        messageReceiver1.shutdown(true);
        // Thread.sleep(5 * 1000);
        logger.info("Channel url should be deleted now");
    }

    private String getChannelUrl(String channelId) {
        return bounceProxyUrl + "channels/" + channelId + "/";
    }

}
