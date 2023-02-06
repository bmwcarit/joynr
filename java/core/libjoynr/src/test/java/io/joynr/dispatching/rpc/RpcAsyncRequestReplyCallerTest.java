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
package io.joynr.dispatching.rpc;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Method;
import java.util.Optional;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ICallback;
import joynr.MethodMetaInformation;
import joynr.Reply;
import joynr.exceptions.ApplicationException;
import joynr.tests.testAsync;
import joynr.tests.testTypes.ErrorEnumBase;

@RunWith(MockitoJUnitRunner.class)
public class RpcAsyncRequestReplyCallerTest {
    private static final String requestReplyId = "testRRID";
    @Mock
    private MethodMetaInformation methodMetaInformation;
    @Mock
    private Future<Void> voidFuture;
    @Mock
    private CallbackWithModeledError<Void, ErrorEnumBase> callbackWithError;
    @Mock
    private Callback<Void> callback;

    private RpcAsyncRequestReplyCaller<Void> create(ICallback callback, Method method) {
        return new RpcAsyncRequestReplyCaller<Void>(null,
                                                    requestReplyId,
                                                    Optional.ofNullable(callback),
                                                    voidFuture,
                                                    method,
                                                    methodMetaInformation);
    }

    @Test
    public void methodWithModelledError_returnsApplicationException() throws NoSuchMethodException {
        doReturn(true).when(methodMetaInformation).hasModelledErrors();

        // prepare reply with ApplicationException
        final ApplicationException exception = new ApplicationException(ErrorEnumBase.BASE_ERROR_TYPECOLLECTION,
                                                                        "TEST: methodWithModelledError_returnsApplicationException");
        Reply reply = new Reply("", exception);

        Method method = testAsync.class.getDeclaredMethod("methodWithErrorEnum", CallbackWithModeledError.class);
        RpcAsyncRequestReplyCaller<Void> replyCaller = create(callbackWithError, method);
        replyCaller.messageCallBack(reply);

        verify(callbackWithError).onFailure(ErrorEnumBase.BASE_ERROR_TYPECOLLECTION);
        verify(voidFuture).onFailure(exception);
        verify(callbackWithError, never()).onSuccess(any());
        verify(voidFuture, never()).onSuccess(any());
        verify(voidFuture, never()).resolve(any());
    }

    @Test
    public void methodWithoutModelledError_doesNotReturnApplicationException() throws NoSuchMethodException {
        doReturn(false).when(methodMetaInformation).hasModelledErrors();

        final ApplicationException exception = new ApplicationException(ErrorEnumBase.BASE_ERROR_TYPECOLLECTION,
                                                                        "TEST: methodWithoutModelledError_doesNotReturnApplicationException");
        final JoynrRuntimeException joynrException = new JoynrRuntimeException("An ApplicationException was received, "
                + "but none was expected. Is the provider version incompatible with the consumer? " + exception);
        Reply reply = new Reply("", exception);

        Method method = testAsync.class.getDeclaredMethod("voidOperation", Callback.class);
        RpcAsyncRequestReplyCaller<Void> replyCaller = create(callback, method);
        replyCaller.messageCallBack(reply);

        verify(callback).onFailure(joynrException);
        verify(voidFuture).onFailure(joynrException);
        verify(callback, never()).onSuccess(any());
        verify(voidFuture, never()).onSuccess(any());
        verify(voidFuture, never()).resolve(any());
    }
}
