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
package io.joynr.jeeintegration.api;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import jakarta.inject.Qualifier;

/**
 * Qualifier annotation for a {@link jakarta.enterprise.inject.Produces producer} method, which returns <code>Properties</code> to use in creating
 * the joynr runtime in the {@link io.joynr.jeeintegration.JoynrRuntimeFactory}.
 * <p>
 * In order to specify your own joynr properties, create a <code>@Singleton</code> EJB which has a method annotated with
 * {@link jakarta.enterprise.inject.Produces} and <code>JoynrProperties</code> and returns a <code>Properties</code> instance. These properties
 * are then added to the default joynr properties and also override these properties as necessary.
 * <p>
 * <b>Note</b> that if the EJB which contains the producer methods implements an interface, then the producer methods
 * also need to be declared in that interface, otherwise CDI won't recognise the method implementations as producers.
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target({ ElementType.METHOD, ElementType.PARAMETER, ElementType.FIELD })
@Qualifier
public @interface JoynrProperties {

}
