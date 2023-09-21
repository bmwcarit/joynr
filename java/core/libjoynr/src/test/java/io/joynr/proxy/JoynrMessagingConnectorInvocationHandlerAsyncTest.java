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
    public void testExecuteAsyncMethodShouldFailIfMethodHasNoCallbackAnnotation() {
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
                                                             any(MessagingQos.class));
        }
    }

    @Test
    public void testExecuteAsyncMethodShouldFailIfDiscoveryEntrySetHasMoreThanOneItem() {
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
                                                             any(MessagingQos.class));
        }
    }

    @Test
    @SuppressWarnings("rawtypes")
    public void testExecuteAsyncMethodShouldSucceed() {
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

        final var requestReplyIdCaptor = ArgumentCaptor.forClass(String.class);
        final var replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
        final var expiryDateCaptor = ArgumentCaptor.forClass(ExpiryDate.class);
        final var requestCaptor = ArgumentCaptor.forClass(Request.class);
        final var messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);

        verify(replyCallerDirectory).addReplyCaller(requestReplyIdCaptor.capture(),
                                                    replyCallerCaptor.capture(),
                                                    expiryDateCaptor.capture());
        verify(requestReplyManager).sendRequest(eq(FROM_PARTICIPANT_ID),
                                                eq(toDiscoveryEntry),
                                                requestCaptor.capture(),
                                                messagingQosCaptor.capture());

        final var requestReplyId = requestReplyIdCaptor.getValue();
        assertNotNull(requestReplyId);
        final var replyCaller = replyCallerCaptor.getValue();
        assertNotNull(replyCaller);
        assertTrue(replyCaller instanceof RpcAsyncRequestReplyCaller);
        assertEquals(proxy, ((RpcAsyncRequestReplyCaller) replyCaller).getProxy());
        final var expiryDate = expiryDateCaptor.getValue();
        assertNotNull(expiryDate);
        assertEquals(60000, expiryDate.getRelativeTtl());
        assertTrue(expiryDate.getValue() > 0);
        final var request = requestCaptor.getValue();
        assertNotNull(request);
        assertEquals(requestReplyId, request.getRequestReplyId());
        assertEquals(method.getName(), request.getMethodName());
        final var messagingQos = messagingQosCaptor.getValue();
        assertNotNull(messagingQos);
        assertEquals(60000, messagingQos.getRoundTripTtl_ms());
    }

    private Method getAsyncMethod(final String methodName, final Class<?>... parameterTypes) {
        return getMethod(TestAsyncInterface.class, methodName, parameterTypes);
    }
}
