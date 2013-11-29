package io.joynr.dispatcher.rpc.annotation;

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

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import com.fasterxml.jackson.core.type.TypeReference;

/**
 * This annotation should be used on parameters of methods that will be invoked using JSON RPC, to specify their names.
 * This is necessary because this information is not stored in release builds.
 *
 * The annotation should usually be used on the interface, because it is also used for stubbing.
 */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.PARAMETER)
public @interface JoynrRpcParam {
    /** The name of the parameter */
    String value();

    /** A type token. This is needed to correctly deserialize collection types because their type information is erased after compilation in Java */
    Class<? extends TypeReference<?>> deserialisationType() default DefaultTypeReference.class;

    /** A default value for the parameter. It is used if a parameter is missing in the function call. This one can only be used for simple types. */
    String defaultValue() default "";

    /** A default value for the parameter as JSON String. It is used if a parameter is missing in the function call. This one can be used for all types. */
    String defaultJsonValue() default "";
}
