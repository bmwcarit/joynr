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
package io.joynr.integration.setup.testrunner;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import io.joynr.integration.setup.BounceProxyServerSetup;

/**
 * Defines for which {@link BounceProxyServerSetup}s the annotated test class
 * should be run. <br>
 * For each setup, before running all test methods,
 * {@link BounceProxyServerSetup#startServers()} will be called. After running
 * all test methods, {@link BounceProxyServerSetup#stopServers()} is called.
 * 
 * @author christina.strobel
 * 
 */
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
public @interface BounceProxyServerSetups {

    Class<? extends BounceProxyServerSetup>[] value();
}
