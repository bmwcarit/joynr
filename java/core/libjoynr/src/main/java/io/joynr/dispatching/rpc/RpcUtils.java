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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

import joynr.MethodMetaInformation;
import joynr.Reply;

public class RpcUtils {
    private static final Logger logger = LoggerFactory.getLogger(RpcUtils.class);

    @Inject
    private static ObjectMapper objectMapper;

    public static Object reconstructReturnedObject(Method method,
                                                   MethodMetaInformation methodMetaInformation,
                                                   Object... response) {

        Object responsePayload = null;

        if (response.length == 1) {
            responsePayload = objectMapper.convertValue(response[0], method.getReturnType());
        } else if (response.length > 1) {
            try {
                convertMultioutResponseToCorrectTypes(method, response);

                final Class<?>[] constructorParameterTypes = { Object[].class };
                responsePayload = method.getReturnType()
                                        .getConstructor(constructorParameterTypes)
                                        .newInstance((Object) response);
            } catch (InstantiationException | IllegalAccessException | IllegalArgumentException
                    | InvocationTargetException | NoSuchMethodException | SecurityException e) {
                logger.error("error calling multi-out method: {}. Unable to recreate return object. Returning NULL instead: "
                        + method.getName(), e);

            }
        }

        return responsePayload;
    }

    public static Object[] reconstructCallbackReplyObject(Method method,
                                                          MethodMetaInformation methodMetaInformation,
                                                          Reply response) {
        if (methodMetaInformation.getCallbackAnnotation() == null) {
            throw new IllegalStateException("Received a reply to a rpc method call without callback annotation including deserializationType");
        }

        int responseParameterCount = response.getResponse().length;
        Object[] responsePayload = null;

        if (responseParameterCount == 0) {
            responsePayload = new Object[0];
        } else if (responseParameterCount == 1) {
            responsePayload = new Object[1];
            try {
                responsePayload[0] = objectMapper.convertValue(response.getResponse()[0],
                                                               methodMetaInformation.getCallbackAnnotation()
                                                                                    .deserializationType());
            } catch (IllegalArgumentException e) {
                logger.error("error calling method: {}. Unable to recreate response for callback. Returning NULL instead"
                        + method.getName(), e);
            }
        } else if (response.getResponse().length > 1) {
            convertMultioutResponseToCorrectTypes(method, response.getResponse());
            responsePayload = response.getResponse();
        }
        return responsePayload;
    }

    public static Object[] convertResponseForStatelessCallbackToCorrectTypes(Method statelessCallbackMethod,
                                                                             Reply reply) {
        Object[] response = reply.getResponse();
        int returnValuesCount = response.length;
        Object[] returnValues = new Object[returnValuesCount];
        Class<?>[] returnValuesTypes = statelessCallbackMethod.getParameterTypes();

        for (int i = 0; i < returnValuesCount; i++) {
            returnValues[i] = objectMapper.convertValue(response[i], returnValuesTypes[i]);
        }

        return returnValues;
    }

    private static void convertMultioutResponseToCorrectTypes(Method method, Object... response) {
        Method getDatatypes;
        try {
            getDatatypes = method.getReturnType().getMethod("getDatatypes");
            Class<?>[] responseDatatypes = (Class<?>[]) getDatatypes.invoke(null);
            for (int i = 0; i < response.length; i++) {
                response[i] = objectMapper.convertValue(response[i], responseDatatypes[i]);
            }
        } catch (NoSuchMethodException | SecurityException | IllegalAccessException | IllegalArgumentException
                | InvocationTargetException e) {
            logger.error("error calling method. Unable to recreate response for callback: Returning NULL instead: "
                    + method.getName(), e);
        }
    }
}
