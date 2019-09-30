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
package io.joynr.runtime;

import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class ShutdownNotifier {

    private static final Logger logger = LoggerFactory.getLogger(ShutdownNotifier.class);

    /**
     * Timeout in seconds after which the {@link #prepareForShutdown()} will stop waiting for the operation to
     * complete and return control to the caller. Note that this doesn't guarantee that the processes triggered
     * by calling {@link #prepareForShutdown()} are also cancelled - they may still be running in the background.
     */
    private static final String PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT = "joynr.runtime.prepareforshutdowntimeout";

    private List<ShutdownListener> shutdownListenerList = new LinkedList<>();

    @Inject(optional = true)
    @Named(PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT)
    private int prepareForShutdownTimeoutSec = 5;

    /**
     * register to have the listener's shutdown method called at system shutdown
     * NOTE: no shutdown order is guaranteed.
     * @param shutdownListener
     */
    public void registerForShutdown(ShutdownListener shutdownListener) {
        synchronized (shutdownListenerList) {
            shutdownListenerList.add(0, shutdownListener);
            logger.trace("#ShutdownListeners: {}", shutdownListenerList.size());
        }
    }

    /**
     * register to have the listener's shutdown method called at system shutdown
     * as one of the last listeners. It is a partial ordering and ensures that this
     * listener's shutdown will be called after all listeners registered using
     * {@link #registerForShutdown(ShutdownListener)}.
     * NOTE: Listeners who manage some executor service should use this method.
     * @param shutdownListener
     */
    public void registerToBeShutdownAsLast(ShutdownListener shutdownListener) {
        synchronized (shutdownListenerList) {
            shutdownListenerList.add(shutdownListener);
        }
    }

    /**
     * Will call {@link ShutdownListener#prepareForShutdown()} for each {@link #registerForShutdown(ShutdownListener) registered listener}
     * asynchronously, and waiting a total of five seconds for all to complete or will then timeout without waiting.
     */
    public void prepareForShutdown() {
        Collection<CompletableFuture<Void>> prepareShutdownFutures;
        synchronized (shutdownListenerList) {
            prepareShutdownFutures = shutdownListenerList.stream()
                                                         .map(shutdownListener -> CompletableFuture.runAsync(() -> shutdownListener.prepareForShutdown()))
                                                         .collect(Collectors.toList());
        }
        try {
            CompletableFuture.allOf(prepareShutdownFutures.toArray(new CompletableFuture[prepareShutdownFutures.size()]))
                             .get(prepareForShutdownTimeoutSec, TimeUnit.SECONDS);
        } catch (Exception e) {
            logger.error("Exception occurred while preparing shutdown.", e);
        }
    }

    /**
     * Calls {@link ShutdownListener#shutdown()} for each {@link #registerForShutdown(ShutdownListener) registered listener}
     * synchronously in turn.
     */
    public void shutdown() {
        synchronized (shutdownListenerList) {
            shutdownListenerList.forEach(shutdownListener -> {
                logger.trace("shutting down {}", shutdownListener);
                try {
                    shutdownListener.shutdown();
                } catch (Exception e) {
                    logger.error("error shutting down {}: {}", shutdownListener, e.getMessage());
                }
            });
        }
    }

    public void unregister(ShutdownListener shutdownListener) {
        synchronized (shutdownListenerList) {
            shutdownListenerList.remove(shutdownListener);
            logger.trace("Removed ShutdownListener, #ShutdownListeners: {}", shutdownListenerList.size());
        }
    }
}
