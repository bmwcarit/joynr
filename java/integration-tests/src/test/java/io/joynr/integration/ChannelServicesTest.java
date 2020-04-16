/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.integration;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertEquals;

import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.jayway.restassured.response.Response;

import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.SingleBounceProxy;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.integration.setup.testrunner.BounceProxyServerSetups;
import io.joynr.integration.setup.testrunner.MultipleBounceProxySetupsTestRunner;
import io.joynr.integration.util.BounceProxyCommunicationMock;
import joynr.ImmutableMessage;

@Ignore("HTTP not supported at the moment")
@RunWith(MultipleBounceProxySetupsTestRunner.class)
@BounceProxyServerSetups(value = { SingleBounceProxy.class })
public class ChannelServicesTest {

    private static final Logger logger = LoggerFactory.getLogger(ChannelServicesTest.class);

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    private String receiverId = "channelservicestest-" + createUuidString();

    private BounceProxyCommunicationMock bpMock;

    @Before
    public void setUp() {
        bpMock = new BounceProxyCommunicationMock(configuration.getAnyBounceProxyUrl(), receiverId);
    }

    @Test
    public void testCreateChannel() throws Exception {
        String channelId = createUuidString();
        bpMock.createChannel(channelId);
    }

    @Test
    public void testCreateChannelWithNoChannelId() throws Exception {
        bpMock.onrequest().with().queryParam("ccid", "").expect().statusCode(400).when().post("channels/");
    }

    @Test
    public void testOpenChannelWithEmptyCache() throws Exception {
        String channelId = createUuidString();
        bpMock.createChannel(channelId);

        int timeout_ms = 1000;
        long timeStart = System.currentTimeMillis();
        Future<Response> longPoll;
        try {
            longPoll = bpMock.longPollInOwnThread(channelId, timeout_ms);
            longPoll.get();
        } catch (ExecutionException e) {
            if (e.getCause() instanceof SocketTimeoutException) {
                logger.info("Long poll finished with expected timeout");
            } else {
                assertThat("invalid exception was thrown: " + e.getMessage(), false);
            }
        }

        long timeTook = System.currentTimeMillis() - timeStart;
        assertThat(timeTook + 1, greaterThan((long) timeout_ms));

    }

    @Test
    public void testOpenChannelWithCachedMessages() throws Exception {
        // removed, was only used to view logfiles of a dependency. Not needed anymore.
        // java.util.logging.Logger.getLogger(ProviderServices.class.getName()).setLevel(Level.ALL);
        String channelId = createUuidString();
        bpMock.createChannel(channelId);
        // generate a random payload
        byte[] payload1 = ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
        byte[] payload2 = ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
        long messageRelativeTtlMs = 1000000L;
        bpMock.postMessage(channelId, messageRelativeTtlMs, payload1);
        bpMock.postMessage(channelId, messageRelativeTtlMs, payload2);
        int longpollTimeout_ms = 100000;

        long timeStart = System.currentTimeMillis();
        Future<Response> responseFuture = bpMock.longPollInOwnThread(channelId, longpollTimeout_ms);
        Response response = responseFuture.get();
        int timeTotal = (int) (System.currentTimeMillis() - timeStart);

        // make sure we did not wait the timeout
        assertThat(timeTotal, lessThan(longpollTimeout_ms));

        List<ImmutableMessage> messagesFromResponse = bpMock.getJoynrMessagesFromResponse(response);

        ImmutableMessage receivedPayload1 = messagesFromResponse.get(0);
        ImmutableMessage receivedPayload2 = messagesFromResponse.get(1);

        assertEquals(receivedPayload1.getUnencryptedBody(), payload1);
        assertEquals(receivedPayload2.getUnencryptedBody(), payload2);
    }

    @Test
    public void testCreateAndDeleteChannel() throws Exception {
        int longpollTimeout_ms = 5000;
        String channelId = createUuidString();
        bpMock.createChannel(channelId);

        // start the long poll on the channel, then delete it.
        Future<Response> longPoll = bpMock.longPollInOwnThread(channelId, longpollTimeout_ms);
        Thread.sleep(100);
        bpMock.deleteChannel(channelId, 100, 200);
        // After delete, the long poll should not longer be active, should not have to wait long until it returns empty
        longPoll.get(100L, TimeUnit.MILLISECONDS);

        // generate a random payload
        byte[] payload1 = ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
        byte[] payload2 = ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
        long messageRelativeTtlMs = 1000000L;
        // expect the messages not to be postable since the channel does not exist (get 400 back from server)
        bpMock.postMessage(channelId, messageRelativeTtlMs, payload1, 400);
        bpMock.postMessage(channelId, messageRelativeTtlMs, payload2, 400);

        bpMock.createChannel(channelId);
        // now the posts should work
        bpMock.postMessage(channelId, messageRelativeTtlMs, payload1);
        bpMock.postMessage(channelId, messageRelativeTtlMs, payload2);
        Thread.sleep(2000);

        long timeStart = System.currentTimeMillis();
        longPoll = bpMock.longPollInOwnThread(channelId, longpollTimeout_ms);
        Response response = longPoll.get();
        int timeTotal = (int) (System.currentTimeMillis() - timeStart);

        // make sure we did not wait the timeout
        assertThat(timeTotal, lessThan(longpollTimeout_ms));

        List<ImmutableMessage> messagesFromResponse = bpMock.getJoynrMessagesFromResponse(response);

        ImmutableMessage receivedPayload1 = messagesFromResponse.get(0);
        ImmutableMessage receivedPayload2 = messagesFromResponse.get(1);

        assertEquals(receivedPayload1.getUnencryptedBody(), payload1);
        assertEquals(receivedPayload2.getUnencryptedBody(), payload2);
    }

    @Test
    public void testOpenNonexistentChannelReturnsNoContent() throws Exception {
        String channelId = createUuidString();
        int longpollTimeout_ms = 100000;

        long timeStart = System.currentTimeMillis();
        bpMock.longPollInOwnThread(channelId, longpollTimeout_ms, 400);
        long elapsedTimeInLongPoll = System.currentTimeMillis() - timeStart;
        assertThat("long poll did not abort immediately", elapsedTimeInLongPoll < 5000);
    }

}
