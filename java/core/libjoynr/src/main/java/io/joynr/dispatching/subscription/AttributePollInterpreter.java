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
package io.joynr.dispatching.subscription;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Optional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.JoynrVersion;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.provider.Promise;
import io.joynr.provider.ProviderContainer;
import io.joynr.util.AnnotationUtil;
import joynr.exceptions.MethodInvocationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.types.Version;

public class AttributePollInterpreter {

    private static final Logger logger = LoggerFactory.getLogger(AttributePollInterpreter.class);

    public Optional<Promise<?>> execute(ProviderContainer providerContainer, Method method) {
        String interfaceName = providerContainer.getInterfaceName();
        Object returnValueFromProvider = null;
        try {
            returnValueFromProvider = method.invoke(providerContainer.getProviderProxy());
        } catch (IllegalAccessException e) {
            String message = String.format("Method \"%s\" is not accessible on \"%s\" provider (exception: \"%s\").",
                                           method.getName(),
                                           interfaceName,
                                           e.toString());
            logger.error(message, e);
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(providerContainer.getProviderProxy().getClass(),
                                                                     JoynrVersion.class);
            throw new MethodInvocationException(message, new Version(joynrVersion.major(), joynrVersion.minor()));
        } catch (IllegalArgumentException e) {
            String message = String.format("Provider of interface \"%s\" does not declare method \"%s\" (exception: \"%s\")",
                                           interfaceName,
                                           method.getName(),
                                           e.toString());
            logger.error(message, e);
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(providerContainer.getProviderProxy().getClass(),
                                                                     JoynrVersion.class);
            throw new MethodInvocationException(message, new Version(joynrVersion.major(), joynrVersion.minor()));
        } catch (InvocationTargetException e) {
            Throwable cause = e.getCause();
            String message = String.format("Calling method \"%s\" on \"%s\" provider threw an exception: \"%s\"",
                                           method.getName(),
                                           interfaceName,
                                           cause == null ? e.toString() : cause.toString());
            logger.error(message, e);
            throw new ProviderRuntimeException(cause == null ? e.toString() : cause.toString());
        } catch (Exception e) {
            String message = String.format("Calling method \"%s\" on \"%s\" provider threw an unexpected exception: \"%s\"",
                                           method.getName(),
                                           interfaceName,
                                           e.toString());
            logger.error(message, e);
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(providerContainer.getProviderProxy().getClass(),
                                                                     JoynrVersion.class);
            throw new MethodInvocationException(message, new Version(joynrVersion.major(), joynrVersion.minor()));
        }

        if (returnValueFromProvider == null) {
            String message = String.format("Calling method \"%s\" on \"%s\" provider returned \"null\".",
                                           method.getName(),
                                           interfaceName);
            logger.error(message);
            throw new JoynrRuntimeException(message);
        }

        Promise<?> returnedPromiseFromProvider = null;
        try {
            returnedPromiseFromProvider = (Promise<?>) returnValueFromProvider;
        } catch (ClassCastException e) {
            String message = String.format("Calling method \"%s\" on \"%s\" provider did not return a promise.",
                                           method.getName(),
                                           interfaceName);
            logger.error(message, e);
            throw new JoynrRuntimeException(message, e);
        }
        return Optional.ofNullable(returnedPromiseFromProvider);
    }
}
