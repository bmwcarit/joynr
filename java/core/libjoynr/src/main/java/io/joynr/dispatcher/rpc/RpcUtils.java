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

import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import javax.annotation.CheckForNull;

import joynr.MethodMetaInformation;
import joynr.Reply;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class RpcUtils {

    @Inject
    private static ObjectMapper objectMapper;

    @CheckForNull
    public static Object reconstructReturnedObject(Method method,
                                                   MethodMetaInformation methodMetaInformation,
                                                   Reply response) throws InstantiationException,
                                                                  IllegalAccessException {

        Object responsePayload = null;

        if (response.getResponse().size() == 1) {
            JoynrRpcReturn annotation = methodMetaInformation.getReturnAnnotation();
            if (annotation == null) {
                responsePayload = objectMapper.convertValue(response.getResponse().get(0), method.getReturnType());
            } else {
                responsePayload = objectMapper.convertValue(response.getResponse().get(0), annotation.deserialisationType().newInstance());
            }
        } else if (response.getResponse().size() > 1) {
            try {
                final Class<?>[] constructorParameterTypes = { Object[].class };
                Object[] parameters = { response.getResponse().toArray() };
                responsePayload = method.getReturnType()
                                        .getConstructor(constructorParameterTypes)
                                        .newInstance(parameters);
            } catch (NoSuchMethodException | InstantiationException | IllegalAccessException | IllegalArgumentException | InvocationTargetException | SecurityException e) {
                // multi-out containers always have a constructor accepting an object array
                System.err.println("multiout constructor is corrupted (no object array accepted as parameter)");
            }
        }

        return responsePayload;
    }

    @CheckForNull
    public static Object reconstructCallbackReplyObject(Method method,
                                                        MethodMetaInformation methodMetaInformation,
                                                        Reply response) throws InstantiationException,
                                                                       IllegalAccessException {
        if (methodMetaInformation.getCallbackAnnotation() == null) {
            throw new IllegalStateException("Received a reply to a rpc method call without callback annotation including deserialisationType");
        }

        Object responsePayload = null;

        if (response.getResponse().size() == 1) {
            responsePayload = response.getResponse().get(0);
        }

        return objectMapper.convertValue(responsePayload, methodMetaInformation.getCallbackAnnotation()
                                                                               .deserialisationType()
                                                                               .newInstance());
    }
}
