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

import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.vehicle.DefaultRadioProvider;

public class RadioServiceProvider extends DefaultRadioProvider {
    private static final Logger logger = LoggerFactory.getLogger(RadioServiceProvider.class);

    @Override
    public Promise<DeferredVoid> shuffleStations() {
        logger.info("Java provider - Shuffle stations called.");

        try {
            Thread.sleep(1000); //simulate time-consuming computation
        } catch (InterruptedException e) {
            logger.info("Provider interrupted");
        }

        DeferredVoid deferredVoid = new DeferredVoid();
        deferredVoid.resolve();
        return new Promise<DeferredVoid>(deferredVoid);
    }
}
