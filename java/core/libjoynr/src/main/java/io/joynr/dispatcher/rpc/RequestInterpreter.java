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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import joynr.Reply;
import joynr.Request;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class RequestInterpreter {
    private static final Logger logger = LoggerFactory.getLogger(RequestInterpreter.class);
    private ObjectMapper objectMapper;

    @Inject
    public RequestInterpreter(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;

    }

    // use for caching because creation of MethodMetaInformation is expensive
    // ConcurrentMap<ClassName, ConcurrentMap<MethodName, MethodMetaInformation>
    // >
    // TODO move methodMetaInformation to a central place, save metaInformations in a cache (e.g. with predefined max
    // size)
    private final ConcurrentMap<MethodSignature, Method> methodSignatureToMethodMap = new ConcurrentHashMap<MethodSignature, Method>();

    private Reply createReply(Request request, Object response) {
        return new Reply(request.getRequestReplyId(), response);
    }

    public void execute(final Callback<Reply> callback, RequestCaller requestCaller, final Request request) {
        Promise<? extends AbstractDeferred> promise = (Promise<?>) invokeMethod(requestCaller, request);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrRuntimeException error) {
                callback.onFailure(error);
            }

            @Override
            public void onFulfillment(Object... values) {
                Object response = null;
                if (values.length > 0) {
                    response = values[0];
                }
                callback.onSuccess(createReply(request, response));
            }
        });

    }

    private Object invokeMethod(RequestCaller requestCaller, Request request) {
        // A method is identified by its defining request caller, its name and the types of its arguments
        MethodSignature methodSignature = new MethodSignature(requestCaller,
                                                              request.getMethodName(),
                                                              request.getFullyQualifiedParamDatatypes());

        ensureMethodMetaInformationPresent(request, methodSignature);

        Method method = methodSignatureToMethodMap.get(methodSignature);

        Object[] params = null;

        if (method.getParameterTypes().length > 0) {
            // method with parameters
            params = request.getParams();
            Class<?>[] parameterTypes = method.getParameterTypes();
            for (int i = 0; i < params.length; i++) {
                params[i] = objectMapper.convertValue(params[i], parameterTypes[i]);
            }
        }

        try {
            return method.invoke(requestCaller, params);
        } catch (IllegalAccessException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non public method {}", request);
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            Throwable cause = e.getCause();
            logger.error("RequestInterpreter: Could not perform an RPC invocation: {}", cause.getMessage());
            throw new RuntimeException(e);
        }
    }

    private void ensureMethodMetaInformationPresent(Request request, MethodSignature methodSignature) {
        try {
            if (!methodSignatureToMethodMap.containsKey(methodSignature)) {
                Method method;
                method = ReflectionUtils.findMethodByParamTypeNames(methodSignature.getRequestCaller().getClass(),
                                                                    methodSignature.getMethodName(),
                                                                    methodSignature.getFullyQualifiedParameterTypeNames());
                methodSignatureToMethodMap.putIfAbsent(methodSignature, method);
            }
        } catch (NoSuchMethodException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non existing method {}", request);
            throw new RuntimeException(e);
        }
    }

}
