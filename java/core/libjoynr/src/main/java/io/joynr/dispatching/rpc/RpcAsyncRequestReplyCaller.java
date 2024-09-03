/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import java.lang.reflect.Method;
import java.util.Optional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import io.joynr.proxy.ICallback;
import io.joynr.proxy.ICallbackWithModeledError;
import joynr.MethodMetaInformation;
import joynr.Reply;
import joynr.exceptions.ApplicationException;

public class RpcAsyncRequestReplyCaller<T> implements ReplyCaller {

    private ICallback callback;
    private Method method;
    private MethodMetaInformation methodMetaInformation;
    private Future<T> future;
    private static final Logger logger = LoggerFactory.getLogger(RpcAsyncRequestReplyCaller.class);
    private String requestReplyId;
    private Object proxy;

    public RpcAsyncRequestReplyCaller(Object proxy,
                                      String requestReplyId,
                                      Optional<ICallback> callback,
                                      Future<T> future,
                                      Method method,
                                      MethodMetaInformation methodMetaInformation) {
        this.proxy = proxy;
        this.requestReplyId = requestReplyId;
        if (callback.isPresent()) {
            this.callback = callback.get();
        }
        this.future = future;
        this.method = method;
        this.methodMetaInformation = methodMetaInformation;
    }

    @Override
    public void messageCallBack(Reply payload) {

        Object[] response = null;
        try {
            if (payload.getError() != null) {
                if (logger.isTraceEnabled()) {
                    logger.trace("REQUEST returns error: requestReplyId: {}, method {}, response:",
                                 requestReplyId,
                                 method.getName(),
                                 (Exception) payload.getError());
                } else {
                    logger.debug("REQUEST returns error: requestReplyId: {}, method {}, response: {}",
                                 requestReplyId,
                                 method.getName(),
                                 payload.getError().toString());
                }
                // Callback must be called first before releasing the future
                errorCallback(payload.getError());
                if (future != null) {
                    if (payload.getError() instanceof ApplicationException
                            && !methodMetaInformation.hasModelledErrors()) {
                        String message = "An ApplicationException was received, but none was expected."
                                + " Is the provider version incompatible with the consumer? " + payload.getError();
                        logger.debug(message);
                        future.onFailure(new JoynrRuntimeException(message));
                    } else {
                        future.onFailure(payload.getError());
                    }
                }

            } else {
                response = RpcUtils.reconstructCallbackReplyObject(method, methodMetaInformation, payload);
                if (logger.isTraceEnabled()) {
                    logger.trace("REQUEST returns successful: requestReplyId: {}, method {}, response: {}",
                                 requestReplyId,
                                 method.getName(),
                                 response);
                } else {
                    logger.debug("REQUEST returns successful: requestReplyId: {}, method {}, response: [not available with current loglevel]",
                                 requestReplyId,
                                 method.getName());
                }
                // Callback must be called first before releasing the future
                if (callback != null) {
                    try {
                        callback.resolve(response);
                    } catch (Exception e) {
                        logger.error("Exception was thrown during execution of callback: {}", e.getMessage());
                    }
                }

                if (future != null) {
                    future.resolve(response);
                }
            }
        } catch (Exception e) {
            logger.error("Error calling async method {}: ", method.getName(), e);
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

        errorCallback(joynrException);

        if (future != null) {
            future.onFailure(joynrException);
        }
    }

    @SuppressWarnings({ "rawtypes", "unchecked" })
    private void errorCallback(JoynrException error) {
        if (callback != null) {
            try {
                if (error instanceof JoynrRuntimeException) {
                    callback.onFailure((JoynrRuntimeException) error);
                } else if (error instanceof ApplicationException) {
                    if (callback instanceof ICallbackWithModeledError) {
                        ((ICallbackWithModeledError) callback).onFailure(((ApplicationException) error).getError());
                    } else {
                        callback.onFailure(new JoynrRuntimeException("An ApplicationException was received, but none was expected."
                                + " Is the provider version incompatible with the consumer? " + error));
                    }
                } else {
                    callback.onFailure(new JoynrRuntimeException("unexpected exception type received: ",
                                                                 (Throwable) error));
                }
            } catch (Exception e) {
                logger.error("Exception was thrown during execution of callback: {}", e.getMessage());
            }
        }
    }

    public Object getProxy() {
        return proxy;
    }
}
