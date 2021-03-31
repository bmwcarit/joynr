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

import java.io.Serializable;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;

import io.joynr.JoynrVersion;
import io.joynr.context.JoynrMessageScope;
import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.dispatching.RequestCaller;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.JoynrMessageCreator;
import io.joynr.messaging.JoynrMessageMetaInfo;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.CallContext;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.provider.ProviderCallback;
import io.joynr.proxy.MethodSignature;
import io.joynr.util.AnnotationUtil;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.MethodInvocationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.types.Version;

public class RequestInterpreter {

    private static final Logger logger = LoggerFactory.getLogger(RequestInterpreter.class);

    private JoynrMessageScope joynrMessageScope;

    private Provider<JoynrMessageCreator> joynrMessageCreatorProvider;

    private Provider<JoynrMessageMetaInfo> joynrMessageContext;

    @Inject
    public RequestInterpreter(JoynrMessageScope joynrMessageScope,
                              Provider<JoynrMessageCreator> joynrMessageCreatorProvider,
                              Provider<JoynrMessageMetaInfo> joynrMessageContext) {
        this.joynrMessageScope = joynrMessageScope;
        this.joynrMessageCreatorProvider = joynrMessageCreatorProvider;
        this.joynrMessageContext = joynrMessageContext;
    }

    // use for caching because creation of MethodMetaInformation is expensive
    // ConcurrentMap<ClassName, ConcurrentMap<MethodName, MethodMetaInformation>
    // >
    // TODO move methodMetaInformation to a central place, save metaInformations in a cache (e.g. with predefined max
    // size)
    private final ConcurrentMap<MethodSignature, Method> methodSignatureToMethodMap = new ConcurrentHashMap<MethodSignature, Method>();

    private Reply createReply(Request request, Object... response) {
        return new Reply(request.getRequestReplyId(), response);
    }

    private String logRequest(Request request) {
        if (logger.isTraceEnabled()) {
            return request.toString();
        } else {
            return String.format("Request: methodName: %s, requestReplyId: %s",
                                 request.getMethodName(),
                                 request.getRequestReplyId());
        }
    }

    private String logRequest(OneWayRequest request) {
        if (logger.isTraceEnabled()) {
            return request.toString();
        } else {
            return String.format("OneWayRequest: methodName: %s", request.getMethodName());
        }
    }

    public void execute(final ProviderCallback<Reply> callback, RequestCaller requestCaller, final Request request) {
        Promise<? extends AbstractDeferred> promise;
        logger.debug("Execute request on provider: {}", logRequest(request));
        try {
            promise = (Promise<?>) invokeMethod(requestCaller, request);
        } catch (MethodInvocationException | ProviderRuntimeException e) {
            logger.warn("Execute request on provider failed with exception: {}, {}", e, request);
            callback.onFailure(e);
            return;
        } catch (Exception e) {
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(requestCaller.getProxy().getClass(),
                                                                     JoynrVersion.class);
            MethodInvocationException methodInvocationException = new MethodInvocationException(e,
                                                                                                new Version(joynrVersion.major(),
                                                                                                            joynrVersion.minor()));
            logger.warn("Execute request on provider failed with exception: {}, {}",
                        methodInvocationException,
                        request);
            callback.onFailure(methodInvocationException);
            return;
        }
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException error) {
                logger.debug("Execute request on provider onRejection: {}, {}", error, logRequest(request));
                callback.onFailure(error);
            }

            @Override
            public void onFulfillment(Object... values) {
                if (logger.isTraceEnabled()) {
                    logger.trace("Execute request on provider onFulfillment: {}, {}", values, request);
                } else {
                    logger.debug("Execute request on provider onFulfillment: {}", logRequest(request));
                }
                callback.onSuccess(createReply(request, values));
            }
        });

    }

    public Object invokeMethod(RequestCaller requestCaller, OneWayRequest request) {
        // A method is identified by its defining request caller, its name and the types of its arguments
        MethodSignature methodSignature = new MethodSignature(requestCaller,
                                                              request.getMethodName(),
                                                              request.getParamDatatypes());

        ensureMethodMetaInformationPresent(requestCaller, request, methodSignature);

        Method method = methodSignatureToMethodMap.get(methodSignature);

        Object[] params = null;
        try {
            if (method.getParameterTypes().length > 0) {
                // method with parameters
                params = request.getParams();
            }
            joynrMessageScope.activate();
            setContext(requestCaller, request);

            logger.trace("Invoke provider method {}({})", method.getName(), params);
            return requestCaller.invoke(method, params);
        } catch (IllegalAccessException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non public method {}",
                         logRequest(request));
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(requestCaller.getProxy().getClass(),
                                                                     JoynrVersion.class);
            throw new MethodInvocationException(e, new Version(joynrVersion.major(), joynrVersion.minor()));

        } catch (InvocationTargetException e) {
            logger.debug("InvokeMethod error", e);
            Throwable cause = e.getCause();
            logger.error("RequestInterpreter: Could not perform an RPC invocation: {}",
                         cause == null ? e.toString() : cause.getMessage());
            throw new ProviderRuntimeException(cause == null ? e.toString() : cause.toString());
        } finally {
            requestCaller.removeContext();
            joynrMessageScope.deactivate();
        }
    }

    private void setContext(RequestCaller requestCaller, OneWayRequest request) {
        String creatorUserId = request.getCreatorUserId();
        Map<String, Serializable> context = request.getContext();

        // Enable guice-scoped
        joynrMessageCreatorProvider.get().setMessageCreatorId(creatorUserId);
        joynrMessageContext.get().setMessageContext(context);
        // allow requestCaller to set thread-local CallContext
        CallContext callContext = new CallContext();
        callContext.setContext(context);
        callContext.setPrincipal(creatorUserId);
        requestCaller.setContext(callContext);
    }

    private void ensureMethodMetaInformationPresent(RequestCaller requestCaller,
                                                    OneWayRequest request,
                                                    MethodSignature methodSignature) {
        try {
            if (!methodSignatureToMethodMap.containsKey(methodSignature)) {
                Method method;
                method = ReflectionUtils.findMethodByParamTypeNames(methodSignature.getRequestCaller()
                                                                                   .getProxy()
                                                                                   .getClass(),
                                                                    methodSignature.getMethodName(),
                                                                    methodSignature.getParameterTypeNames());
                methodSignatureToMethodMap.putIfAbsent(methodSignature, method);
            }
        } catch (NoSuchMethodException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for non existing method in {}. Error:",
                         logRequest(request),
                         e);
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(requestCaller.getProxy().getClass(),
                                                                     JoynrVersion.class);
            throw new MethodInvocationException(e.toString(), new Version(joynrVersion.major(), joynrVersion.minor()));
        }
    }
}
