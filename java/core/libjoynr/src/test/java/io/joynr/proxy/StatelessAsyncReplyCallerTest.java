/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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
package io.joynr.proxy;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.function.Function;

import org.junit.Before;
import org.junit.Test;

import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import joynr.Reply;
import joynr.vehicle.Navigation;
import joynr.vehicle.NavigationStatelessAsync;
import joynr.vehicle.NavigationStatelessAsyncCallback;

public class StatelessAsyncReplyCallerTest {

    private Map<String, Boolean> resultHolder = new HashMap<>();

    public class NavigationCallback implements NavigationStatelessAsyncCallback {

        @Override
        public String getUseCaseName() {
            return "test";
        }

        @Override
        public void requestGuidanceSuccess(Boolean result, String messageId) {
            assertTrue(result);
            resultHolder.put(messageId, true);
        }

        @Override
        public void addTripSuccess(String messageId) {
            resultHolder.put(messageId, true);
        }
    }

    private NavigationCallback callback = new NavigationCallback();

    @Before
    public void setup() {
        resultHolder.clear();
    }

    @Test
    public void testCallAddTrip() throws Exception {
        testCall("addTrip", requestReplyId -> new Reply(requestReplyId));
    }

    @Test
    public void testCallRequestGuidanceSuccess() throws Exception {
        testCall("requestGuidance", requestReplyId -> new Reply(requestReplyId, Boolean.TRUE));
    }

    private void testCall(String methodName, Function<String, Reply> replyGenerator) {
        String statelessAsyncCallbackId = buildStatelessAsyncCallbackId(methodName);
        StatelessAsyncReplyCaller subject = new StatelessAsyncReplyCaller(statelessAsyncCallbackId, callback);
        String requestReplyId = UUID.randomUUID().toString();
        Reply reply = replyGenerator.apply(requestReplyId);
        reply.setStatelessCallback(statelessAsyncCallbackId);
        subject.messageCallBack(reply);
        assertTrue(resultHolder.containsKey(requestReplyId));
        assertTrue(resultHolder.get(requestReplyId));
    }

    private StatelessCallbackCorrelation getStatelessCallbackCorrelation(String methodName) {
        return Arrays.stream(NavigationStatelessAsync.class.getMethods())
                     .filter(method -> method.getName().equals(methodName))
                     .findFirst()
                     .orElseThrow(() -> new RuntimeException("Method " + methodName + " not found on "
                             + NavigationStatelessAsync.class.getName()))
                     .getAnnotation(StatelessCallbackCorrelation.class);
    }

    private String buildStatelessAsyncCallbackId(String methodName) {
        StatelessCallbackCorrelation callbackCorrelation = getStatelessCallbackCorrelation(methodName);
        assertNotNull(callbackCorrelation);
        return Navigation.INTERFACE_NAME + ":~:test:#:" + callbackCorrelation.value();
    }
}
