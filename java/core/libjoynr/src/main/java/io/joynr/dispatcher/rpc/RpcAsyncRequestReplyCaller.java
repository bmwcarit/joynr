package io.joynr.dispatcher.rpc;

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

import io.joynr.dispatcher.ReplyCaller;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrInvalidInnvocationException;
import io.joynr.proxy.Future;

import java.lang.reflect.Method;

import javax.annotation.CheckForNull;

import joynr.MethodMetaInformation;
import joynr.Reply;

public class RpcAsyncRequestReplyCaller<T> implements ReplyCaller {

    @CheckForNull
    private Callback<T> callback;
    private Method method;
    private MethodMetaInformation methodMetaInformation;
    private Future<T> future;
    // private static final Logger logger = LoggerFactory.getLogger(RpcAsyncRequestReplyCaller.class);
    private String requestReplyId;

    public RpcAsyncRequestReplyCaller(String requestReplyId,
                                      @CheckForNull Callback<T> callback,
                                      Future<T> future,
                                      Method method,
                                      MethodMetaInformation methodMetaInformation) {
        this.requestReplyId = requestReplyId;
        this.callback = callback;
        this.future = future;
        this.method = method;
        this.methodMetaInformation = methodMetaInformation;
    }

    @SuppressWarnings("unchecked")
    @Override
    public void messageCallBack(Reply payload) {

        Object reply = null;
        try {
            reply = RpcUtils.reconstructCallbackReplyObject(method, methodMetaInformation, payload);
        } catch (Throwable e) {
            error(new JoynrInvalidInnvocationException(e));
            return;
        }

        try {

            // Callback must be called first before releasing the future
            if (callback != null) {
                callback.onSuccess((T) reply);
            }

            if (future != null) {
                future.onSuccess((T) reply);
            }

        } catch (Throwable e) {
            // ignore
        }
    }

    @Override
    public void error(Throwable error) {
        JoynrRuntimeException joynrRuntimeException;
        // wrap non-joynr exceptions in a JoynrRuntimeException
        if (error instanceof JoynrRuntimeException) {
            joynrRuntimeException = (JoynrRuntimeException) error;
        } else {
            joynrRuntimeException = new JoynrRuntimeException(error);
        }

        if (callback != null) {
            callback.onFailure(joynrRuntimeException);
        }
        if (future != null) {
            future.onFailure(joynrRuntimeException);
        }
    }

    @Override
    public String getRequestReplyId() {
        return requestReplyId;
    }
}
