package io.joynr.dispatching.rpc;

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

import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import javax.annotation.CheckForNull;

import joynr.MethodMetaInformation;
import joynr.Reply;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class RpcUtils {
    private static final Logger logger = LoggerFactory.getLogger(RpcUtils.class);

    @Inject
    private static ObjectMapper objectMapper;

    @CheckForNull
    public static Object reconstructReturnedObject(Method method,
                                                   MethodMetaInformation methodMetaInformation,
                                                   Object... response)  {

        Object responsePayload = null;

            if (response.length == 1) {
                JoynrRpcReturn annotation = methodMetaInformation.getReturnAnnotation();
                if (annotation == null) {
                    responsePayload = objectMapper.convertValue(response[0], method.getReturnType());
                } else {
                    try {
                        responsePayload = objectMapper.convertValue(response[0], annotation.deserializationType().newInstance());
                    } catch (IllegalArgumentException | InstantiationException | IllegalAccessException e) {
                        logger.error("error calling method: {}. Unable to recreate return object: {}. Returning NULL instead", method.getName(), e.getMessage());

                    }
                }
            } else if (response.length > 1) {
                    final Class<?>[] constructorParameterTypes = { Object[].class };
                    try {
                        responsePayload = method.getReturnType()
                                                .getConstructor(constructorParameterTypes)
                                                .newInstance(new Object[]{ response });
                    } catch (InstantiationException | IllegalAccessException | IllegalArgumentException
                            | InvocationTargetException | NoSuchMethodException | SecurityException e) {
                        logger.error("error calling multi-out method: {}. Unable to recreate return object: {}. Returning NULL instead", method.getName(), e.getMessage());

                    }
            }


        return responsePayload;
    }

    @CheckForNull
    public static Object[] reconstructCallbackReplyObject(Method method,
                                                          MethodMetaInformation methodMetaInformation,
                                                          Reply response) {
        if (methodMetaInformation.getCallbackAnnotation() == null) {
            throw new IllegalStateException("Received a reply to a rpc method call without callback annotation including deserializationType");
        }


        int responseParameterCount = response.getResponse().size();
        Object[] responsePayload = null;

        if (responseParameterCount == 0) {
            responsePayload = new Object[0];
        } else if (responseParameterCount == 1) {
            responsePayload = new Object[1];
            try {
                responsePayload[0] = objectMapper.convertValue(response.getResponse().get(0),
                                                               methodMetaInformation.getCallbackAnnotation()
                                                                                    .deserializationType()
                                                                                    .newInstance());
            } catch (IllegalArgumentException | InstantiationException | IllegalAccessException e) {
                logger.error("error calling method: {}. Unable to recreate response for callback: {}. Returning NULL instead", method.getName(), e.getMessage());
            }
        } else if (response.getResponse().size() > 1) {
            responsePayload = response.getResponse().toArray();
        }
        return responsePayload;
    }
}
