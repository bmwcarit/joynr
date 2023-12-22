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
import io.joynr.dispatching.rpc.RpcAsyncRequestReplyCaller;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import joynr.Request;
import joynr.types.DiscoveryEntryWithMetaInfo;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.junit.MockitoJUnitRunner;

import java.lang.reflect.Method;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

@RunWith(MockitoJUnitRunner.class)
public class JoynrMessagingConnectorInvocationHandlerAsyncTest
        extends AbstractJoynrMessagingConnectorInvocationHandlerTest {

    @Test
    public void testExecuteAsyncMethodShouldFailIfMethodHasNoCallbackAnnotation() throws NoSuchMethodException {
        method = getAsyncMethod("someMethodWithoutAnnotations", Integer.class, String.class);
        parameters = new Object[]{ 1, "test" };
        try {
            handler.executeAsyncMethod(proxy, method, parameters, future);
            fail("Exception excepted");
        } catch (final JoynrIllegalStateException exception) {
            assertTrue(exception.getMessage().contains("async method"));
            assertTrue(exception.getMessage().contains("annotated callback parameter"));
            verify(replyCallerDirectory, never()).addReplyCaller(anyString(),
                                                                 any(ReplyCaller.class),
                                                                 any(ExpiryDate.class));
            verify(requestReplyManager, never()).sendRequest(anyString(),
                                                             any(DiscoveryEntryWithMetaInfo.class),
                                                             any(Request.class),
                                                             any(MessagingQos.class),
                                                             any(ExpiryDate.class));
        }
    }

    @Test
    public void testExecuteAsyncMethodShouldFailIfDiscoveryEntrySetHasMoreThanOneItem() throws NoSuchMethodException {
        method = getAsyncMethod("someMethodWithoutAnnotations", Integer.class, String.class);
        parameters = new Object[]{ 1, "test" };
        addNewDiscoveryEntry();
        try {
            handler.executeAsyncMethod(proxy, method, parameters, future);
            fail("Exception excepted");
        } catch (final JoynrIllegalStateException exception) {
            assertTrue(exception.getMessage().contains("multiple participants"));
            verify(replyCallerDirectory, never()).addReplyCaller(anyString(),
                                                                 any(ReplyCaller.class),
                                                                 any(ExpiryDate.class));
            verify(requestReplyManager, never()).sendRequest(anyString(),
                                                             any(DiscoveryEntryWithMetaInfo.class),
                                                             any(Request.class),
                                                             any(MessagingQos.class),
                                                             any(ExpiryDate.class));
        }
    }

    @Test
    @SuppressWarnings("rawtypes")
    public void testExecuteAsyncMethodShouldSucceed() throws NoSuchMethodException {
        method = getAsyncMethod("someMethodWithAnnotation", Integer.class, String.class, Callback.class);
        parameters = new Object[]{ 1, "test", new Callback<Void>() {
            @Override
            public void onFailure(final JoynrRuntimeException runtimeException) {
                // do nothing
            }

            @Override
            public void onSuccess(final Void result) {
                // do nothing
            }
        } };
        handler.executeAsyncMethod(proxy, method, parameters, future);

        final ArgumentCaptor<String> requestReplyIdCaptor = ArgumentCaptor.forClass(String.class);
        final ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
        final ArgumentCaptor<ExpiryDate> replyCallerExpiryDateCaptor = ArgumentCaptor.forClass(ExpiryDate.class);
        final ArgumentCaptor<ExpiryDate> requestReplyManagerExpiryDateCaptor = ArgumentCaptor.forClass(ExpiryDate.class);
        final ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        final ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);

        verify(replyCallerDirectory).addReplyCaller(requestReplyIdCaptor.capture(),
                                                    replyCallerCaptor.capture(),
                                                    replyCallerExpiryDateCaptor.capture());
        verify(requestReplyManager).sendRequest(eq(FROM_PARTICIPANT_ID),
                                                eq(toDiscoveryEntry),
                                                requestCaptor.capture(),
                                                messagingQosCaptor.capture(),
                                                requestReplyManagerExpiryDateCaptor.capture());

        final String requestReplyId = requestReplyIdCaptor.getValue();
        assertNotNull(requestReplyId);
        final ReplyCaller replyCaller = replyCallerCaptor.getValue();
        assertNotNull(replyCaller);
        assertTrue(replyCaller instanceof RpcAsyncRequestReplyCaller);
        assertEquals(proxy, ((RpcAsyncRequestReplyCaller) replyCaller).getProxy());
        final ExpiryDate replyCallerExpiryDate = replyCallerExpiryDateCaptor.getValue();
        assertNotNull(replyCallerExpiryDate);
        assertEquals(60000, replyCallerExpiryDate.getRelativeTtl());
        assertTrue(replyCallerExpiryDate.getValue() > 0);
        final Request request = requestCaptor.getValue();
        assertNotNull(request);
        assertEquals(requestReplyId, request.getRequestReplyId());
        assertEquals(method.getName(), request.getMethodName());
        final MessagingQos messagingQos = messagingQosCaptor.getValue();
        assertNotNull(messagingQos);
        assertEquals(60000, messagingQos.getRoundTripTtl_ms());
        final ExpiryDate requestReplyManagerExpiryDate = replyCallerExpiryDateCaptor.getValue();
        assertNotNull(requestReplyManagerExpiryDate);
        assertEquals(60000, requestReplyManagerExpiryDate.getRelativeTtl());
        assertTrue(requestReplyManagerExpiryDate.getValue() > 0);
        assertEquals(replyCallerExpiryDate, requestReplyManagerExpiryDate);
    }

    private Method getAsyncMethod(final String methodName,
                                  final Class<?>... parameterTypes) throws NoSuchMethodException {
        return getMethod(TestAsyncInterface.class, methodName, parameterTypes);
    }
}
