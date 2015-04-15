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
import io.joynr.dispatcher.RequestCallerAsync;
import io.joynr.dispatcher.RequestCallerSync;
import io.joynr.exceptions.JoynrException;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import joynr.MethodMetaInformation;
import joynr.Reply;
import joynr.Request;

import org.apache.commons.lang.ArrayUtils;
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
    private final ConcurrentMap<String, ConcurrentMap<List<String>, MethodMetaInformation>> methodMetaInformationMap = new ConcurrentHashMap<String, ConcurrentMap<List<String>, MethodMetaInformation>>();

    private Reply createReply(Request request, Object response) {
        return new Reply(request.getRequestReplyId(), response);
    }

    public Reply execute(RequestCallerSync requestCaller, Request request) {
        return createReply(request, invokeMethod(requestCaller, request));
    }

    public void execute(final Callback<Reply> callback, RequestCallerAsync requestCaller, final Request request) {
        Callback<Object> genericCallback = new Callback<Object>() {

            @Override
            public void onSuccess(Object response) {
                callback.onSuccess(createReply(request, response));
            }

            @Override
            public void onFailure(JoynrException error) {
                logger.error("RequestInterpreter: Error occurred during execution of async RPC invocation: {}", error);
                callback.onFailure(error);
            }
        };
        invokeMethod(requestCaller, request, new Object[]{ genericCallback }, new String[]{ Callback.class.getName() });
    }

    private Object invokeMethod(RequestCaller requestCaller, Request request) {
        return invokeMethod(requestCaller, request, new Object[0], new String[0]);
    }

    private Object invokeMethod(RequestCaller requestCaller,
                                Request request,
                                Object[] additionalArguments,
                                String[] additionalArgumentTypes) {
        // A method is identified by its name and the types of its arguments
        List<String> methodSignature = new ArrayList<String>();

        methodSignature.add(request.getMethodName());

        for (String type : additionalArgumentTypes) {
            methodSignature.add(type);
        }
        if (request.hasParamDatatypes()) {
            List<String> paramDatatypes = request.getFullyQualifiedParamDatatypes();
            methodSignature.addAll(paramDatatypes);
        }

        ensureMethodMetaInformationPresent(request, requestCaller, methodSignature);

        MethodMetaInformation methodMetaInformation = methodMetaInformationMap.get(requestCaller.getClass().toString())
                                                                              .get(methodSignature);

        if (methodMetaInformation.isMethodWithoutParameters()) {
            return invokeMethod(request, requestCaller, methodMetaInformation);
        }

        Object[] params = ArrayUtils.addAll(additionalArguments, request.getParams());
        Class<?>[] parameterTypes = methodMetaInformation.getMethod().getParameterTypes();
        for (int i = additionalArguments.length; i < params.length; i++) {
            params[i] = objectMapper.convertValue(params[i], parameterTypes[i]);
        }

        return invokeMethod(request, requestCaller, methodMetaInformation, params);
    }

    private Object invokeMethod(Request request,
                                RequestCaller requestCaller,
                                MethodMetaInformation methodMetaInformation) {
        return invokeMethod(request, requestCaller, methodMetaInformation, null);
    }

    private Object invokeMethod(Request request,
                                RequestCaller requestCaller,
                                MethodMetaInformation methodMetaInformation,
                                Object[] params) {
        try {
            return methodMetaInformation.getMethod().invoke(requestCaller, params);
        } catch (IllegalAccessException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non public method {}", request);
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            Throwable cause = e.getCause();
            logger.error("RequestInterpreter: Could not perform an RPC invocation: {}", cause.getMessage());
            throw new RuntimeException(e);
        }
    }

    private void ensureMethodMetaInformationPresent(Request request,
                                                    RequestCaller requestCaller,
                                                    List<String> methodSignature) {
        try {
            String interfaceName = requestCaller.getClass().toString();
            if (!methodMetaInformationMap.containsKey(interfaceName)) {
                methodMetaInformationMap.putIfAbsent(interfaceName,
                                                     new ConcurrentHashMap<List<String>, MethodMetaInformation>());
            }
            if (!methodMetaInformationMap.get(interfaceName).containsKey(methodSignature)) {
                Method method;
                method = ReflectionUtils.findMethodByParamTypeNames(requestCaller.getClass(),
                                                                    methodSignature.get(0),
                                                                    methodSignature.subList(1, methodSignature.size()));
                methodMetaInformationMap.get(interfaceName).putIfAbsent(methodSignature,
                                                                        new MethodMetaInformation(method));
            }
        } catch (NoSuchMethodException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non existing method {}", request);
            throw new RuntimeException(e);
        } catch (IOException e) {
            logger.error("RequestInterpreter: IOException: {}", e.getMessage());
            throw new RuntimeException(e);
        }
    }

}
