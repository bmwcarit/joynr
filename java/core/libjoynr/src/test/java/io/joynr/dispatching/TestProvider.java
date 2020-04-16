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
package io.joynr.dispatching;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.Promise;
import joynr.Request;
import joynr.tests.DefaulttestProvider;

public class TestProvider extends DefaulttestProvider {

    // maps from request payload to response
    private Map<Object, Object> sentPayloads = new HashMap<Object, Object>();
    private static final Logger logger = LoggerFactory.getLogger(TestProvider.class);
    private WaitTillCondition waitTillCondition;

    public TestProvider(int numberOfMessagesExpected) {
        waitTillCondition = new WaitTillCondition(numberOfMessagesExpected) {

            @Override
            protected Collection<Object> getReceivedPayloads() {
                return sentPayloads.keySet();
            }
        };
    }

    @Override
    public Promise<MethodWithStringsDeferred> methodWithStrings(String payload) {

        MethodWithStringsDeferred deferred = new MethodWithStringsDeferred();
        logger.info("Responding to payload: {}", payload.toString());
        String response = "response to " + payload.toString();
        sentPayloads.put(payload, response);

        waitTillCondition.releaseSemaphorePermit();
        deferred.resolve(response);
        return new Promise<>(deferred);
    }

    public Collection<Object> getSentPayloads() {
        return sentPayloads.values();
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

    public void waitForMessage(int timeOutMs) {
        waitTillCondition.waitForMessage(timeOutMs);
    }

    public void assertAllPayloadsReceived(int timeOutMs) {
        waitTillCondition.assertAllPayloadsReceived(timeOutMs);
    }

    public void assertReceivedPayloadsContainsNot(String payloads) {
        waitTillCondition.assertReceivedPayloadsContainsNot(payloads);
    }

    public void assertReceivedPayloadsContains(String payloads) {
        waitTillCondition.assertReceivedPayloadsContains(payloads);
    }
}
