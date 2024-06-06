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
 * Used to specify the local joynr domain to use during application and runtime creation if the default should not be
 * used.
 * <p>
 * Similar to {@link JoynrProperties}, this is used to decorate a producer method. The decorated method must return a
 * <code>String</code> value, which will then be used as the local domain of the joynr runtime.
 * <p>
 * <b>Note</b> that if the EJB which contains the producer methods implements an interface, then the producer methods
 * also need to be declared in that interface, otherwise CDI won't recognise the method implementations as producers.
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target({ ElementType.METHOD, ElementType.PARAMETER, ElementType.FIELD })
@Qualifier
public @interface JoynrLocalDomain {

}
