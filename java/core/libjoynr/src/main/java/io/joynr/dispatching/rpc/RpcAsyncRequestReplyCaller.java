package io.joynr.dispatching.rpc;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import io.joynr.proxy.ICallback;

import java.lang.reflect.Method;

import javax.annotation.CheckForNull;

import joynr.MethodMetaInformation;
import joynr.Reply;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class RpcAsyncRequestReplyCaller<T> implements ReplyCaller {

    @CheckForNull
    private ICallback callback;
    private Method method;
    private MethodMetaInformation methodMetaInformation;
    private Future<T> future;
    private static final Logger logger = LoggerFactory.getLogger(RpcAsyncRequestReplyCaller.class);
    private String requestReplyId;

    public RpcAsyncRequestReplyCaller(String requestReplyId,
                                      @CheckForNull ICallback callback,
                                      Future<T> future,
                                      Method method,
                                      MethodMetaInformation methodMetaInformation) {
        this.requestReplyId = requestReplyId;
        this.callback = callback;
        this.future = future;
        this.method = method;
        this.methodMetaInformation = methodMetaInformation;
    }

    @Override
    public void messageCallBack(Reply payload) {

        Object[] response = null;
        try {
            if (payload.getError() != null) {
                // Callback must be called first before releasing the future
                if (callback != null) {
                    callback.onFailure(payload.getError());
                }

                if (future != null) {
                    future.onFailure(payload.getError());
                }
            } else {
                response = RpcUtils.reconstructCallbackReplyObject(method, methodMetaInformation, payload);
                // Callback must be called first before releasing the future
                if (callback != null) {
                    callback.resolve(response);
                }

                if (future != null) {
                    future.resolve(response);
                }
            }
        } catch (Exception e) {
            logger.error("Error calling async method: {} error: {}", method.getName(), e.getMessage());
        }

    }

    @Override
    public void error(Throwable error) {
        JoynrException joynrException;
        // wrap non-joynr exceptions in a JoynrRuntimeException
        if (error instanceof JoynrException) {
            joynrException = (JoynrException) error;
        } else {
            joynrException = new JoynrRuntimeException(error);
        }

        if (callback != null) {
            callback.onFailure(joynrException);
        }
        if (future != null) {
            future.onFailure(joynrException);
        }
    }

    @Override
    public String getRequestReplyId() {
        return requestReplyId;
    }
}
