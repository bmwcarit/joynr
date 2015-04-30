package io.joynr.pubsub.publication;

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
import io.joynr.provider.Promise;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import javax.annotation.Nonnull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AttributePollInterpreter {

    private static final Logger logger = LoggerFactory.getLogger(AttributePollInterpreter.class);

    @Nonnull
    public Promise<?> execute(RequestCaller requestCaller, Method method) {
        Object returnValueFromProvider = null;
        try {
            returnValueFromProvider = method.invoke(requestCaller);
        } catch (IllegalAccessException e) {
            String message = String.format("Method \"%s\" is not accessible on \"%s\" provider.",
                                           method.getName(),
                                           requestCaller.getInterfaceName());
            logger.error(message, e);
            throw new JoynrRuntimeException(message, e);
        } catch (IllegalArgumentException e) {
            String message = String.format("Provider of interface \"%s\" does not declare method \"%s\"",
                                           requestCaller.getInterfaceName(),
                                           method.getName());
            logger.error(message, e);
            throw new JoynrRuntimeException(message, e);
        } catch (InvocationTargetException e) {
            String message = String.format("Calling method \"%s\" on \"%s\" provider throwed an exception.",
                                           method.getName(),
                                           requestCaller.getInterfaceName());
            logger.error(message, e);
            throw new JoynrRuntimeException(message, e);
        }

        if (returnValueFromProvider == null) {
            String message = String.format("Calling method \"%s\" on \"%s\" provider returned \"null\".",
                                           method.getName(),
                                           requestCaller.getInterfaceName());
            logger.error(message);
            throw new JoynrRuntimeException(message);
        }

        Promise<?> returnedPromiseFromProvider = null;
        try {
            returnedPromiseFromProvider = (Promise<?>) returnValueFromProvider;
        } catch (ClassCastException e) {
            String message = String.format("Calling method \"%s\" on \"%s\" provider did not return a promise.",
                                           method.getName(),
                                           requestCaller.getInterfaceName());
            logger.error(message, e);
            throw new JoynrRuntimeException(message, e);
        }
        return returnedPromiseFromProvider;
    }
}
