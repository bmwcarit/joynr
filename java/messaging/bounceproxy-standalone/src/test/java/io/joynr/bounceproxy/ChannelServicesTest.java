package io.joynr.bounceproxy;

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

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.lessThan;
import static org.hamcrest.text.IsEqualIgnoringCase.equalToIgnoringCase;
import io.joynr.messaging.util.Utilities;

import java.net.SocketTimeoutException;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;

import org.junit.Test;

import com.jayway.restassured.response.Response;

public class ChannelServicesTest extends BounceProxyTest {

    @Test
    public void testCreateChannel() throws Exception {
        String channelId = UUID.randomUUID().toString();
        createChannel(channelId);

    }

    @Test
    public void testCreateChannelWithNoChannelId() throws Exception {
        onrequest().with().queryParam("ccid", "").expect().statusCode(400).when().post("/bounceproxy/channels/");

    }

    @Test
    public void testOpenChannelWithEmptyCache() throws Exception {
        String channelId = UUID.randomUUID().toString();
        createChannel(channelId);

        int timeout_ms = 1000;
        long timeStart = System.currentTimeMillis();
        Future<Response> longPoll;
        try {
            longPoll = longPoll(channelId, timeout_ms);
            longPoll.get();
        } catch (ExecutionException e) {
            if (e.getCause() instanceof SocketTimeoutException) {
                logger.info("long poll finished with expected timeout");
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
        String channelId = UUID.randomUUID().toString();
        createChannel(channelId);
        // generate a random payload
        String payload1 = "payload-" + UUID.randomUUID().toString();
        String payload2 = "payload-" + UUID.randomUUID().toString();
        long messageRelativeTtlMs = 1000000L;
        postMessage(channelId, messageRelativeTtlMs, payload1);
        postMessage(channelId, messageRelativeTtlMs, payload2);
        int longpollTimeout_ms = 100000;

        long timeStart = System.currentTimeMillis();
        Future<Response> responseFuture = longPoll(channelId, longpollTimeout_ms);
        Response response = responseFuture.get();
        String payloadReceived = response.getBody().asString();
        int timeTotal = (int) (System.currentTimeMillis() - timeStart);

        // make sure we did not wait the timeout
        assertThat(timeTotal, lessThan(longpollTimeout_ms));
        List<String> splitJson = Utilities.splitJson(payloadReceived);

        JoynrMessage receivedPayload1 = objectMapper.readValue(splitJson.get(0), JoynrMessage.class);
        JoynrMessage receivedPayload2 = objectMapper.readValue(splitJson.get(1), JoynrMessage.class);

        assertThat("payload1: ", receivedPayload1.getPayload(), equalToIgnoringCase(payload1));
        assertThat("payload2: ", receivedPayload2.getPayload(), equalToIgnoringCase(payload2));
    }

    @Test
    public void testCreateAndDeleteChannel() throws Exception {
        int longpollTimeout_ms = 5000;
        String channelId = UUID.randomUUID().toString();
        createChannel(channelId);

        // start the long poll on the channel, then delete it.
        Future<Response> longPoll = longPoll(channelId, longpollTimeout_ms);
        Thread.sleep(100);
        deleteChannel(channelId);
        // After delete, the long poll should not longer be active, should not have to wait long until it returns empty
        longPoll.get(100L, TimeUnit.MILLISECONDS);

        // generate a random payload
        String payload1 = "payload-" + UUID.randomUUID().toString();
        String payload2 = "payload-" + UUID.randomUUID().toString();
        long messageRelativeTtlMs = 1000000L;
        // expect the messages not to be postable since the channel does not exist (get 400 back from server)
        postMessage(channelId, messageRelativeTtlMs, payload1, 400);
        postMessage(channelId, messageRelativeTtlMs, payload2, 400);

        createChannel(channelId);
        // now the posts should work
        postMessage(channelId, messageRelativeTtlMs, payload1);
        postMessage(channelId, messageRelativeTtlMs, payload2);
        Thread.sleep(2000);

        long timeStart = System.currentTimeMillis();
        longPoll = longPoll(channelId, longpollTimeout_ms);
        Response response = longPoll.get();
        String payloadReceived = response.getBody().asString();
        int timeTotal = (int) (System.currentTimeMillis() - timeStart);

        // make sure we did not wait the timeout
        assertThat(timeTotal, lessThan(longpollTimeout_ms));
        List<String> splitJson = Utilities.splitJson(payloadReceived);

        JoynrMessage receivedPayload1 = objectMapper.readValue(splitJson.get(0), JoynrMessage.class);
        JoynrMessage receivedPayload2 = objectMapper.readValue(splitJson.get(1), JoynrMessage.class);

        assertThat("payload1: ", receivedPayload1.getPayload(), equalToIgnoringCase(payload1));
        assertThat("payload2: ", receivedPayload2.getPayload(), equalToIgnoringCase(payload2));
    }

    @Test
    public void testOpenNonexistentChannelReturnsNoContent() throws Exception {
        String channelId = UUID.randomUUID().toString();
        int longpollTimeout_ms = 100000;

        long timeStart = System.currentTimeMillis();
        longPoll(channelId, longpollTimeout_ms, 400);
        long elapsedTimeInLongPoll = System.currentTimeMillis() - timeStart;
        assertThat("long poll did not abort immediately", elapsedTimeInLongPoll < 5000);
    }

}
