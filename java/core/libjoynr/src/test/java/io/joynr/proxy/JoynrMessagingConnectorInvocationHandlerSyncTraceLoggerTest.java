/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.rpc.MultiReturnValuesContainer;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.messaging.MessagingQos;
import joynr.MethodMetaInformation;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;
import org.mockito.Mockito;

import java.lang.reflect.Method;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.verify;

public class JoynrMessagingConnectorInvocationHandlerSyncTraceLoggerTest
        extends AbstractJoynrMessagingConnectorInvocationHandlerSyncTest {
    @Override
    protected boolean isTraceEnabled() {
        return true;
    }

    @Test
    public void testExecuteSyncMethodThatReturnsMultiValueShouldSucceed() {
        method = getSyncMethod("methodReturningMultiValues");
        parameters = new Object[]{};
        final var expectedResult = new TestMultiResult();
        mockSendSyncRequest(expectedResult);

        try (MockedStatic<RpcUtils> mockRpcUtils = Mockito.mockStatic(RpcUtils.class)) {
            mockRpcUtils.when(() -> RpcUtils.reconstructReturnedObject(any(Method.class),
                                                                       any(MethodMetaInformation.class),
                                                                       any())).thenReturn(expectedResult);
            try {
                final var traceMsgParamsCaptor = ArgumentCaptor.forClass(Object[].class);
                doNothing().when(loggerMock).trace(anyString(), traceMsgParamsCaptor.capture());

                final var result = handler.executeSyncMethod(method, parameters);
                assertNotNull(result);
                assertTrue(result instanceof TestMultiResult);
                assertEquals(expectedResult, result);

                final var traceMessageParams = traceMsgParamsCaptor.getAllValues();
                assertNotNull(traceMessageParams);
                assertEquals(3, traceMessageParams.size());
                final var responseString = (Object) traceMessageParams.get(2);
                assertNotNull(responseString);
                //noinspection ConstantConditions
                assertTrue(responseString instanceof String);
                assertTrue(((String) responseString).contains(TestMultiResult.VALUE_A));
                assertTrue(((String) responseString).contains(TestMultiResult.VALUE_B));

                verify(replyCallerDirectory).addReplyCaller(anyString(), any(ReplyCaller.class), any(ExpiryDate.class));
                verify(requestReplyManager).sendSyncRequest(eq(FROM_PARTICIPANT_ID),
                                                            eq(toDiscoveryEntry),
                                                            any(Request.class),
                                                            any(SynchronizedReplyCaller.class),
                                                            any(MessagingQos.class));
            } catch (final ApplicationException exception) {
                fail("Unexpected exception: " + exception.getMessage());
                throw new RuntimeException(exception);
            }
        }
    }

    static class TestMultiResult implements MultiReturnValuesContainer {

        public static final String VALUE_A = "[VALUE_A]";
        public static final String VALUE_B = "[VALUE_B]";

        @Override
        public Object[] getValues() {
            return new Object[]{ VALUE_A, VALUE_B };
        }
    }
}
