package io.joynr.dispatcher.rpc;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class JsonRequestInterpreter {
    private static final Logger logger = LoggerFactory.getLogger(JsonRequestInterpreter.class);
    private ObjectMapper objectMapper;

    @Inject
    public JsonRequestInterpreter(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;

    }

    // use for caching because creation of MethodMetaInformation is expensive
    // ConcurrentMap<ClassName, ConcurrentMap<MethodName, MethodMetaInformation>
    // >
    // TODO move methodMetaInformation to a central place, save metaInformations in a cache (e.g. with predefined max
    // size)
    private final ConcurrentMap<String, ConcurrentMap<List<String>, MethodMetaInformation>> methodMetaInformationMap = new ConcurrentHashMap<String, ConcurrentMap<List<String>, MethodMetaInformation>>();

    public Reply execute(RequestCaller requestCaller, Request jsonRequest) {
        Object response = invokeMethodAndHandleExceptions(requestCaller, jsonRequest);

        return new Reply(jsonRequest.getRequestReplyId(), response);

    }

    private Object invokeMethodAndHandleExceptions(RequestCaller requestCaller, Request jsonRequest) {
        try {
            return invokeMethod(requestCaller, jsonRequest);
        } catch (NoSuchMethodException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non existing method {}", jsonRequest);
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non public method {}", jsonRequest);
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            Throwable cause = e.getCause();
            logger.error("RequestInterpreter: Could not perform an RPC invocation: {}", cause.getMessage());
            throw new RuntimeException(e);
        } catch (IOException e) {
            logger.error("RequestInterpreter: IOException: {}", e.getMessage());
            throw new RuntimeException(e);
        }
    }

    private Object invokeMethod(RequestCaller requestCaller, Request request) throws NoSuchMethodException,
                                                                             InvocationTargetException,
                                                                             IllegalAccessException, IOException {

        // A method is identified by its name and the types of its arguments
        List<String> methodSignature = new ArrayList<String>();
        methodSignature.add(request.getMethodName());
        if (request.hasParamDatatypes()) {
            List<String> paramDatatypes = request.getFullyQualifiedParamDatatypes();
            methodSignature.addAll(paramDatatypes);
        }

        ensureMethodMetaInformationPresent(requestCaller, methodSignature);

        MethodMetaInformation methodMetaInformation = methodMetaInformationMap.get(requestCaller.getClass().toString())
                                                                              .get(methodSignature);

        if (methodMetaInformation.isMethodWithoutParameters()) {
            return methodMetaInformation.getMethod().invoke(requestCaller);
        }

        Object[] params = request.getParams();
        Class<?>[] parameterTypes = methodMetaInformation.getMethod().getParameterTypes();
        for (int i = 0; i < params.length; i++) {
            params[i] = objectMapper.convertValue(params[i], parameterTypes[i]);
        }

        return methodMetaInformation.getMethod().invoke(requestCaller, params);

    }

    private void ensureMethodMetaInformationPresent(RequestCaller requestCaller, List<String> methodSignature)
                                                                                                              throws NoSuchMethodException,
                                                                                                              JsonMappingException {
        String interfaceName = requestCaller.getClass().toString();
        if (!methodMetaInformationMap.containsKey(interfaceName)) {
            methodMetaInformationMap.putIfAbsent(interfaceName,
                                                 new ConcurrentHashMap<List<String>, MethodMetaInformation>());
        }
        if (!methodMetaInformationMap.get(interfaceName).containsKey(methodSignature)) {
            Method method = ReflectionUtils.findMethodByParamTypeNames(requestCaller.getClass(),
                                                                       methodSignature.get(0),
                                                                       methodSignature.subList(1,
                                                                                               methodSignature.size()));
            methodMetaInformationMap.get(interfaceName).putIfAbsent(methodSignature, new MethodMetaInformation(method));
        }
    }
}
