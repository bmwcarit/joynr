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

import static io.joynr.proxy.StatelessAsyncIdCalculator.USE_CASE_SEPARATOR;
import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.types.Localisation.GetTripErrors;
import joynr.types.Localisation.Trip;
import org.junit.Before;
import org.junit.Test;

import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import joynr.Reply;
import joynr.vehicle.Navigation;
import joynr.vehicle.NavigationStatelessAsync;
import joynr.vehicle.NavigationStatelessAsyncCallback;

public class StatelessAsyncReplyCallerTest {

    private static final String STATELESS_CALLBACK_ID = Navigation.INTERFACE_NAME + USE_CASE_SEPARATOR + "test";
    private static final String INVALID = "invalid";

    private Map<String, Boolean> resultHolder = new HashMap<>();

    public class NavigationCallback implements NavigationStatelessAsyncCallback {

        @Override
        public String getUseCase() {
            return "test";
        }

        @Override
        public void requestGuidanceSuccess(Boolean result, ReplyContext replyContext) {
            assertTrue(result);
            resultHolder.put(replyContext.getMessageId(), true);
        }

        @Override
        public void addTripSuccess(ReplyContext replyContext) {
            resultHolder.put(replyContext.getMessageId(), true);
        }

        @Override
        public void updateTripFailed(Navigation.UpdateTripErrorEnum error, ReplyContext replyContext) {
            resultHolder.put(replyContext.getMessageId() + "_error", true);
        }

        @Override
        public void updateTripFailed(JoynrRuntimeException runtimeException, ReplyContext replyContext) {
            resultHolder.put(replyContext.getMessageId() + "_exception", true);
        }

        @Override
        public void deleteTripFailed(JoynrRuntimeException runtimeException, ReplyContext replyContext) {
            resultHolder.put(replyContext.getMessageId(), true);
        }

        @StatelessCallbackCorrelation(INVALID)
        public void invalidMethod(ReplyContext replyContext) {
            fail("Should never be called");
        }

        @Override
        public void getTripFailed(GetTripErrors error, ReplyContext replyContext) {
            resultHolder.put(replyContext.getMessageId(), true);
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

    @Test
    public void testUpdateTripWithError() throws Exception {
        testCall("updateTrip",
                 requestReplyId -> new Reply(requestReplyId,
                                             new ApplicationException(Navigation.UpdateTripErrorEnum.UNKNOWN_TRIP)),
                 requestReplyId -> requestReplyId + "_error");
    }

    @Test
    public void testUpdateTripWithException() throws Exception {
        testCall("updateTrip",
                 requestReplyId -> new Reply(requestReplyId, new ProviderRuntimeException("test")),
                 requestReplyId -> requestReplyId + "_exception");
    }

    @Test
    public void testOverriddenDeleteTripFailedWithException() throws Exception {
        StatelessCallbackCorrelation statelessCallbackCorrelation = getStatelessCallbackCorrelation("deleteTrip",
                                                                                                    Trip.class,
                                                                                                    MessageIdCallback.class);
        testCall(statelessCallbackCorrelation,
                 requestReplyId -> new Reply(requestReplyId, new ProviderRuntimeException("test overloaded methods")),
                 Function.identity());
    }

    @Test(expected = JoynrRuntimeException.class)
    public void testMethodNotFound() throws Exception {
        StatelessAsyncReplyCaller subject = new StatelessAsyncReplyCaller(STATELESS_CALLBACK_ID, callback);
        Reply reply = new Reply(createUuidString());
        reply.setStatelessAsyncCallbackId(STATELESS_CALLBACK_ID);
        reply.setStatelessAsyncCallbackMethodId("0");
        subject.messageCallBack(reply);
    }

    @Test(expected = JoynrRuntimeException.class)
    public void testMethodNameWrong() throws Exception {
        StatelessAsyncReplyCaller subject = new StatelessAsyncReplyCaller(STATELESS_CALLBACK_ID, callback);
        Reply reply = new Reply(createUuidString());
        reply.setStatelessAsyncCallbackId(STATELESS_CALLBACK_ID);
        reply.setStatelessAsyncCallbackMethodId(INVALID);
        subject.messageCallBack(reply);
    }

    @Test
    public void testMethodOverloadWithErrorEnumReferenceAndDifferentOutputParams() throws Exception {
        StatelessCallbackCorrelation statelessCallbackCorrelation = getStatelessCallbackCorrelation("getTrip",
                                                                                                    String.class,
                                                                                                    Boolean.class,
                                                                                                    MessageIdCallback.class);
        testCall(statelessCallbackCorrelation,
                 requestReplyId -> new Reply(requestReplyId,
                                             new ApplicationException(GetTripErrors.NO_MATCHING_TRIP_FOUND)),
                 Function.identity());
        statelessCallbackCorrelation = getStatelessCallbackCorrelation("getTrip",
                                                                       String.class,
                                                                       MessageIdCallback.class);
        testCall(statelessCallbackCorrelation,
                 requestReplyId -> new Reply(requestReplyId, new ApplicationException(GetTripErrors.UNKNOWN_TRIP)),
                 Function.identity());
    }

    private void testCall(String methodName, Function<String, Reply> replyGenerator) {
        testCall(methodName, replyGenerator, Function.identity());
    }

    private void testCall(String methodName,
                          Function<String, Reply> replyGenerator,
                          Function<String, String> requestReplyIdKeyMapper) {
        testCall(getStatelessCallbackCorrelation(methodName), replyGenerator, requestReplyIdKeyMapper);
    }

    private void testCall(StatelessCallbackCorrelation statelessCallbackCorrelation,
                          Function<String, Reply> replyGenerator,
                          Function<String, String> requestReplyIdKeyMapper) {
        StatelessAsyncReplyCaller subject = new StatelessAsyncReplyCaller(STATELESS_CALLBACK_ID, callback);
        String requestReplyId = createUuidString();
        Reply reply = replyGenerator.apply(requestReplyId);
        reply.setStatelessAsyncCallbackId(STATELESS_CALLBACK_ID);
        reply.setStatelessAsyncCallbackMethodId(statelessCallbackCorrelation.value());
        subject.messageCallBack(reply);
        String resultHolderKey = requestReplyIdKeyMapper.apply(requestReplyId);
        assertTrue(resultHolder.containsKey(resultHolderKey));
        assertTrue(resultHolder.get(resultHolderKey));
    }

    private StatelessCallbackCorrelation getStatelessCallbackCorrelation(String methodName) {
        return Arrays.stream(NavigationStatelessAsync.class.getMethods())
                     .filter(method -> method.getName().equals(methodName))
                     .findFirst()
                     .orElseThrow(() -> new RuntimeException("Method " + methodName + " not found on "
                             + NavigationStatelessAsync.class.getName()))
                     .getAnnotation(StatelessCallbackCorrelation.class);
    }

    private StatelessCallbackCorrelation getStatelessCallbackCorrelation(String methodName,
                                                                         Class<?>... parameterTypes) throws Exception {
        Method method = NavigationStatelessAsync.class.getMethod(methodName, parameterTypes);
        StatelessCallbackCorrelation result = method.getAnnotation(StatelessCallbackCorrelation.class);
        assertNotNull("Method named " + methodName + " with parameters " + Arrays.toString(parameterTypes)
                + " has not stateless callback correlation annotation.", result);
        return result;
    }

}
