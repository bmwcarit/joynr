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
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.util.ObjectMapper;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.types.DiscoveryEntryWithMetaInfo;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;
import org.slf4j.Logger;
// CHECKSTYLE IGNORE IllegalImport FOR NEXT 1 LINES
import sun.misc.Unsafe;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.UUID;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(MockitoJUnitRunner.class)
public abstract class AbstractJoynrMessagingConnectorInvocationHandlerSyncTest
        extends AbstractJoynrMessagingConnectorInvocationHandlerTest {

    protected abstract boolean isTraceEnabled();

    @Mock
    protected Logger loggerMock;

    @Override
    public void setUp() {
        super.setUp();
        mockLogger(isTraceEnabled());
    }

    @Test
    public void testExecuteSyncMethodWithoutParametersShouldSucceed() throws NoSuchMethodException {
        method = getSyncMethod("methodWithoutParameters");
        parameters = new Object[]{};
        mockSendSyncRequest(new Object());

        try {
            final Object result = handler.executeSyncMethod(method, parameters);
            assertNull(result);

            final ArgumentCaptor<String> requestReplyIdCaptor = ArgumentCaptor.forClass(String.class);
            final ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
            final ArgumentCaptor<SynchronizedReplyCaller> syncReplyCallerCaptor = ArgumentCaptor.forClass(SynchronizedReplyCaller.class);
            final ArgumentCaptor<ExpiryDate> replyCallerExpiryDateCaptor = ArgumentCaptor.forClass(ExpiryDate.class);
            final ArgumentCaptor<ExpiryDate> requestReplyManagerExpiryDateCaptor = ArgumentCaptor.forClass(ExpiryDate.class);
            final ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);

            verify(replyCallerDirectory).addReplyCaller(requestReplyIdCaptor.capture(),
                                                        replyCallerCaptor.capture(),
                                                        replyCallerExpiryDateCaptor.capture());
            verify(requestReplyManager).sendSyncRequest(eq(FROM_PARTICIPANT_ID),
                                                        eq(toDiscoveryEntry),
                                                        requestCaptor.capture(),
                                                        syncReplyCallerCaptor.capture(),
                                                        any(MessagingQos.class),
                                                        requestReplyManagerExpiryDateCaptor.capture());

            final String requestReplyId = requestReplyIdCaptor.getValue();
            assertNotNull(requestReplyId);
            final ReplyCaller replyCaller = replyCallerCaptor.getValue();
            assertNotNull(replyCaller);
            assertTrue(replyCaller instanceof SynchronizedReplyCaller);
            final ExpiryDate replyCallerExpiryDate = replyCallerExpiryDateCaptor.getValue();
            assertNotNull(replyCallerExpiryDate);
            assertEquals(60000, replyCallerExpiryDate.getRelativeTtl());
            assertTrue(replyCallerExpiryDate.getValue() > 0);
            final Request request = requestCaptor.getValue();
            assertNotNull(request);
            assertEquals(requestReplyId, request.getRequestReplyId());
            final SynchronizedReplyCaller syncReplyCaller = syncReplyCallerCaptor.getValue();
            assertNotNull(syncReplyCaller);
            assertEquals(replyCaller, syncReplyCaller);
            final ExpiryDate requestReplyManagerExpiryDate = requestReplyManagerExpiryDateCaptor.getValue();
            assertEquals(60000, requestReplyManagerExpiryDate.getRelativeTtl());
            assertTrue(requestReplyManagerExpiryDate.getValue() > 0);
            assertEquals(replyCallerExpiryDate, requestReplyManagerExpiryDate);
        } catch (final ApplicationException exception) {
            fail("Unexpected exception: " + exception.getMessage());
        }
    }

    @Test
    public void testExecuteSyncMethodWithoutParametersShouldThrowJoynrRuntimeExceptionOnApplicationException() throws NoSuchMethodException {
        method = getSyncMethod("methodWithoutParameters");
        parameters = new Object[]{};
        mockSendSyncRequestError(new ApplicationException(ApplicationErrors.ERROR_VALUE_2));

        try {
            handler.executeSyncMethod(method, parameters);
            fail("An exception is expected");
        } catch (final ApplicationException exception) {
            fail("Unexpected exception: " + exception.getMessage());
        } catch (final JoynrRuntimeException exception) {
            assertNotNull(exception);
            assertNotNull(exception.getMessage());
        }
    }

    @Test
    public void testExecuteSyncMethodWithoutParametersShouldFailIfDiscoveryEntrySetHasMoreThanOneItem() throws NoSuchMethodException {
        method = getSyncMethod("methodWithoutParameters");
        parameters = new Object[]{};
        addNewDiscoveryEntry();
        try {
            handler.executeSyncMethod(method, parameters);
            fail("Exception excepted");
        } catch (final ApplicationException exception) {
            fail("Unexpected exception: " + exception.getMessage());
        } catch (final JoynrIllegalStateException exception) {
            assertTrue(exception.getMessage().contains("multiple participants"));
            verify(requestReplyManager, never()).sendRequest(anyString(),
                                                             any(DiscoveryEntryWithMetaInfo.class),
                                                             any(Request.class),
                                                             any(MessagingQos.class),
                                                             any(ExpiryDate.class));
        }
    }

    @Test
    public void testExecuteSyncMethodWithoutParametersShouldThrowApplicationExceptionOnApplicationException() throws NoSuchMethodException {
        method = getSyncMethod("methodWithoutParametersWithModelledErrors");
        parameters = new Object[]{};
        mockSendSyncRequestError(new ApplicationException(ApplicationErrors.ERROR_VALUE_1));

        try {
            handler.executeSyncMethod(method, parameters);
            fail("An exception is expected");
        } catch (final ApplicationException exception) {
            assertNotNull(exception);
            assertNotNull(exception.getMessage());
        }
    }

    @Test
    public void testExecuteSyncMethodWithoutParametersShouldThrowJoynrRuntimeExceptionOnJoynrRuntimeException() throws NoSuchMethodException {
        method = getSyncMethod("methodWithoutParameters");
        parameters = new Object[]{};
        mockSendSyncRequestError(new JoynrRuntimeException("Oops"));

        try {
            handler.executeSyncMethod(method, parameters);
            fail("An exception is expected");
        } catch (final ApplicationException exception) {
            fail("Unexpected exception: " + exception.getMessage());
        } catch (final JoynrRuntimeException exception) {
            assertNotNull(exception);
            assertNotNull(exception.getMessage());
        }
    }

    @Test
    public void testExecuteSyncMethodWithParametersAndResultsShouldSucceed() throws NoSuchMethodException {
        method = getSyncMethod("methodWithParameters", Integer.class, Integer.class);
        parameters = new Object[]{};
        final Object expectedResult = Integer.valueOf(42);
        mockSendSyncRequest(expectedResult);
        setUpRpcUtils();

        try {
            final Object result = handler.executeSyncMethod(method, parameters);
            assertNotNull(result);
            assertEquals(expectedResult, result);

            verify(replyCallerDirectory).addReplyCaller(anyString(), any(ReplyCaller.class), any(ExpiryDate.class));
            verify(requestReplyManager).sendSyncRequest(eq(FROM_PARTICIPANT_ID),
                                                        eq(toDiscoveryEntry),
                                                        any(Request.class),
                                                        any(SynchronizedReplyCaller.class),
                                                        any(MessagingQos.class),
                                                        any(ExpiryDate.class));
        } catch (final ApplicationException exception) {
            fail("Unexpected exception: " + exception.getMessage());
        }
    }

    protected void mockSendSyncRequest(final Object response) {
        mockSendSyncRequest(new Reply(UUID.randomUUID().toString(), response));
    }

    protected void mockSendSyncRequestError(final JoynrException exception) {
        mockSendSyncRequest(new Reply(UUID.randomUUID().toString(), exception));
    }

    protected void mockSendSyncRequest(final Reply reply) {
        when(requestReplyManager.sendSyncRequest(any(String.class),
                                                 any(DiscoveryEntryWithMetaInfo.class),
                                                 any(Request.class),
                                                 any(SynchronizedReplyCaller.class),
                                                 any(MessagingQos.class),
                                                 any(ExpiryDate.class))).thenReturn(reply);
    }

    protected Method getSyncMethod(final String methodName,
                                   final Class<?>... parameterTypes) throws NoSuchMethodException {
        return getMethod(TestSyncInterface.class, methodName, parameterTypes);
    }

    protected void mockLogger(final boolean traceEnabled) {
        when(loggerMock.isTraceEnabled()).thenReturn(traceEnabled);
        final Field field = getField(handler.getClass(), "logger");
        setFieldValue(field, loggerMock);
    }

    protected void setUpRpcUtils() {
        final Field field = getField(RpcUtils.class, "objectMapper");
        setFieldValue(field, new ObjectMapper());
    }

    private Field getField(@SuppressWarnings("rawtypes") final Class aClass, final String fieldName) {
        try {
            return aClass.getDeclaredField(fieldName);
        } catch (final NoSuchFieldException exception) {
            throw new RuntimeException(exception);
        }
    }

    private void setFieldValue(final Field field, final Object value) {
        try {
            final Field unsafeField = Unsafe.class.getDeclaredField("theUnsafe");
            unsafeField.setAccessible(true);
            final Unsafe unsafe = (Unsafe) unsafeField.get(null);

            final Object staticFieldBase = unsafe.staticFieldBase(field);
            final long staticFieldOffset = unsafe.staticFieldOffset(field);
            unsafe.putObject(staticFieldBase, staticFieldOffset, value);
        } catch (final NoSuchFieldException | IllegalAccessException exception) {
            throw new RuntimeException(exception);
        }
    }
}
