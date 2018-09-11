/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.examples.messagepersistence.java;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.runtime.AbstractJoynrApplication;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class MessagePersistenceJavaApplication extends AbstractJoynrApplication {
    private static final Logger logger = LoggerFactory.getLogger(MessagePersistenceJavaApplication.class);

    @Override
    public void run() {
        logger.info("Java provider application running ...");
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        RadioServiceProvider radioServiceProvider = new RadioServiceProvider();
        runtime.registerProvider("io.joynr.examples.messagepersistence", radioServiceProvider, providerQos);

        while (true) {
            try {
                Thread.sleep(1000L);
            } catch (InterruptedException e) {
                logger.error("Interrupted while looping.", e);
            }
        }
    }

}
