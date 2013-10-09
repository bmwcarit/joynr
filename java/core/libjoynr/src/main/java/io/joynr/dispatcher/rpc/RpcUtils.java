package io.joynr.dispatcher.rpc;

/*
 * #%L
 * joynr::java::messaging::messagingcommon
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

import java.lang.reflect.Method;

import joynr.MethodMetaInformation;
import joynr.Reply;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class RpcUtils {

    @Inject
    private static ObjectMapper objectMapper;

    public static Object reconstructReturnedObject(Method method,
                                                   MethodMetaInformation methodMetaInformation,
                                                   Reply response) throws InstantiationException,
                                                                  IllegalAccessException {
        JoynrRpcReturn annotation = methodMetaInformation.getReturnAnnotation();

        // TODO IMPORTANT how to handle a method that returns null?
        if (annotation == null) {
            return objectMapper.convertValue(response.getResponse(), method.getReturnType());
        }

        return objectMapper.convertValue(response.getResponse(), annotation.deserialisationType().newInstance());

    }

    public static Object reconstructCallbackReplyObject(Method method,
                                                        MethodMetaInformation methodMetaInformation,
                                                        Reply response) throws InstantiationException,
                                                                       IllegalAccessException {
        if (methodMetaInformation.getCallbackAnnotation() == null) {
            throw new IllegalStateException("Received a reply to a rpc method call without callback annotation including deserialisationType");
        }

        return objectMapper.convertValue(response.getResponse(), methodMetaInformation.getCallbackAnnotation()
                                                                                      .deserialisationType()
                                                                                      .newInstance());

    }
}
