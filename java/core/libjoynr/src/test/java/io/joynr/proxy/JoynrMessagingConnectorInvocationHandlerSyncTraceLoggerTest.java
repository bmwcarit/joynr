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
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.messaging.MessagingQos;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import org.junit.Test;
import org.mockito.ArgumentCaptor;

import java.util.List;

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
    public void testExecuteSyncMethodThatReturnsMultiValueShouldSucceed() throws NoSuchMethodException {
        method = getSyncMethod("methodReturningMultiValues");
        parameters = new Object[]{};
        final TestSyncInterface.TestMultiResult expectedResult = new TestSyncInterface.TestMultiResult();
        mockSendSyncRequest(expectedResult);
        setUpRpcUtils();

        try {
            final ArgumentCaptor<Object[]> traceMsgParamsCaptor = ArgumentCaptor.forClass(Object[].class);
            doNothing().when(loggerMock).trace(anyString(), traceMsgParamsCaptor.capture());

            final Object result = handler.executeSyncMethod(method, parameters);
            assertNotNull(result);
            assertTrue(result instanceof TestSyncInterface.TestMultiResult);
            //noinspection deprecation
            assertEquals(expectedResult.getValues(), ((TestSyncInterface.TestMultiResult) result).getValues());

            final List<?> traceMessageParams = traceMsgParamsCaptor.getAllValues();
            assertNotNull(traceMessageParams);
            assertEquals(3, traceMessageParams.size());
            final Object responseString = traceMessageParams.get(2);
            assertNotNull(responseString);
            //noinspection ConstantConditions
            assertTrue(responseString instanceof String);
            assertTrue(((String) responseString).contains(TestSyncInterface.TestMultiResult.VALUE_A));
            assertTrue(((String) responseString).contains(TestSyncInterface.TestMultiResult.VALUE_B));

            verify(replyCallerDirectory).addReplyCaller(anyString(), any(ReplyCaller.class), any(ExpiryDate.class));
            verify(requestReplyManager).sendSyncRequest(eq(FROM_PARTICIPANT_ID),
                                                        eq(toDiscoveryEntry),
                                                        any(Request.class),
                                                        any(SynchronizedReplyCaller.class),
                                                        any(MessagingQos.class));
        } catch (final ApplicationException exception) {
            fail("Unexpected exception: " + exception.getMessage());
        }
    }

}
