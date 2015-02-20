package io.joynr.dispatcher;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.pubsub.publication.AttributeListener;
import io.joynr.pubsub.publication.BroadcastListener;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import joynr.Request;
import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TestRequestCaller extends WaitTillCondition implements RequestCallerSync {

    // maps from request payload to response
    private Map<Object, Object> sentPayloads = new HashMap<Object, Object>();
    private static final Logger logger = LoggerFactory.getLogger(RequestReplyDispatcherImpl.class);

    public TestRequestCaller(int numberOfMessagesExpected) {
        super(numberOfMessagesExpected);
    }

    public String respond(@JoynrRpcParam("payload") String payload) {
        logger.info("Responding to payload: " + payload.toString());
        String response = "response to " + payload.toString();
        sentPayloads.put(payload, response);

        releaseSemaphorePermit();
        return response;
    }

    public Collection<Object> getSentPayloads() {
        return sentPayloads.values();
    }

    @Override
    protected Collection<Object> getReceivedPayloads() {
        return sentPayloads.keySet();
    }

    public String getSentPayloadFor(String payload) {
        return sentPayloads.get(payload).toString();
    }

    public Object getSentPayloadFor(Request request) {
        String payload = request.getParams()[0].toString();
        Object sentPayload = sentPayloads.get(payload);
        return sentPayload;
    }

    public void assertSentPayloadsContains(Object... payloads) {
        assertEquals(payloads.length, getSentPayloads().size());
        for (Object payload : payloads) {
            assertTrue(getSentPayloads().contains(payload));
        }
    }

    @Override
    public ProviderQos getProviderQos() {
        return new ProviderQos();
    }

    @Override
    public void registerAttributeListener(String attributeName, AttributeListener attributeListener) {
        // TODO Auto-generated method stub

    }

    @Override
    public void unregisterAttributeListener(String attributeName, AttributeListener attributeListener) {
        // TODO Auto-generated method stub

    }

    @Override
    public void registerBroadcastListener(String broadcastName, BroadcastListener broadcastListener) {
        // TODO Auto-generated method stub

    }

    @Override
    public void unregisterBroadcastListener(String broadcastName, BroadcastListener broadcastListener) {
        // TODO Auto-generated method stub

    }
}
