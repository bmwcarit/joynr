/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.examples.spring.provider;

import com.google.inject.Injector;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.exceptions.ApplicationException;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ProviderApp extends AbstractJoynrApplication {

    private static final Logger logger = LoggerFactory.getLogger(ProviderApp.class);

    private RadioProvider provider;
    private Injector injector;

    private boolean working = false;

    @Override
    public void run() {
        provider = new RadioProvider(injector);
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(System.currentTimeMillis());
        providerQos.setScope(ProviderScope.GLOBAL);
        final Future<Void> future = runtime.getProviderRegistrar(localDomain, provider)
                                           .withProviderQos(providerQos)
                                           .awaitGlobalRegistration()
                                           .register();
        try {
            future.get();
        } catch (final JoynrRuntimeException | ApplicationException | InterruptedException e) {
            logger.error("runtime.registerProvider failed: ", e);
            return;
        }
        working = true;

        while (working) {
            logger.info("Provider log entry");
            try {
                Thread.sleep(1000L);
            } catch (final InterruptedException e) {
                throw new RuntimeException(e);
            }
        }
    }

    public void shuffleStations() {
        if (working) {
            this.provider.shuffleStations();
        } else {
            throw new IllegalStateException("No provider");
        }
    }

    public int getCurrentRadioStationInvocationCount() {
        if (working) {
            return provider.getGetCurrentStationInvocationCount();
        } else {
            throw new IllegalStateException("No provider");
        }
    }

    public int getShuffleRadioStationsInvocationCount() {
        if (working) {
            return provider.getShuffleStationsInvocationCount();
        } else {
            throw new IllegalStateException("No provider");
        }
    }

    public void setInjector(final Injector injector) {
        this.injector = injector;
    }
}
