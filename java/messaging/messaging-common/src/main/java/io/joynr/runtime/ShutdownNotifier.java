/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2023 BMW Car IT GmbH
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

    private static final String HIVEMQ_MQTT_CLIENT_FACTORY_CLASS_NAME = "io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory";
    private static final String MESSAGE_TRACKER_FOR_GRACEFUL_SHUTDOWN_CLASS_NAME = "io.joynr.messaging.tracking.MessageTrackerForGracefulShutdown";
    private static final String PROXY_INVOCATION_HANDLER_CLASS_NAME = "io.joynr.proxy.ProxyInvocationHandler";

    private static final String UNEXPECTED_CLASS_MESSAGE = "Listener expected to be of class: %s, but is of class %s";
    private static final String DEDICATED_METHOD_REGISTRATION_REQUIRED_MESSAGE = "Use dedicated method to register this listener";

    private final List<PrepareForShutdownListener> prepareForShutdownListenerList = new LinkedList<>();
    private final List<ShutdownListener> shutdownListenerList = new LinkedList<>();

    // Explicit injection of PrepareForShutdownListener and ShutdownListener interfaces.
    // This would allow us to call them in specific order.
    private PrepareForShutdownListener hivemqMqttPrepareForShutdownListener;
    private PrepareForShutdownListener messageTrackerPrepareForShutdownListener;
    private ShutdownListener hivemqMqttShutdownListener;
    private ShutdownListener messageTrackerShutdownListener;
    private final List<PrepareForShutdownListener> proxyInvocationHandlerPrepareForShutdownListenerList = new LinkedList<>();
    private final List<ShutdownListener> proxyInvocationHandlerShutdownListenerList = new LinkedList<>();

    @Inject(optional = true)
    @Named(PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT)
    private int prepareForShutdownTimeoutSec = 5;

    /**
     * Register to have HivemqMqttClientFactory.prepareForShutdown()
     * called at system prepareForShutdown.
     * @param listener PrepareForShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerHivemqMqttPrepareForShutdownListener(PrepareForShutdownListener listener) throws IllegalArgumentException {
        final String className = listener.getClass().getName();
        if (!className.equals(HIVEMQ_MQTT_CLIENT_FACTORY_CLASS_NAME))
            throw new IllegalArgumentException(String.format(UNEXPECTED_CLASS_MESSAGE,
                                                             HIVEMQ_MQTT_CLIENT_FACTORY_CLASS_NAME,
                                                             className));

        hivemqMqttPrepareForShutdownListener = listener;
    }

    /**
     * Register to have MessageTrackerForGracefulShutdown.prepareForShutdown()
     * called at system prepareForShutdown.
     * @param listener PrepareForShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerMessageTrackerPrepareForShutdownListener(PrepareForShutdownListener listener) throws IllegalArgumentException {
        final String className = listener.getClass().getName();
        if (!className.equals(MESSAGE_TRACKER_FOR_GRACEFUL_SHUTDOWN_CLASS_NAME))
            throw new IllegalArgumentException(String.format(UNEXPECTED_CLASS_MESSAGE,
                                                             MESSAGE_TRACKER_FOR_GRACEFUL_SHUTDOWN_CLASS_NAME,
                                                             className));

        messageTrackerPrepareForShutdownListener = listener;
    }

    /**
     * Register to have ProxyInvocationHandler.prepareForShutdown()
     * called at system shutdown.
     * @param listener PrepareForShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerProxyInvocationHandlerPrepareForShutdownListener(PrepareForShutdownListener listener) throws IllegalArgumentException {
        final String className = listener.getClass().getName();
        if (!className.contains(PROXY_INVOCATION_HANDLER_CLASS_NAME))
            throw new IllegalArgumentException(String.format(UNEXPECTED_CLASS_MESSAGE,
                                                             PROXY_INVOCATION_HANDLER_CLASS_NAME,
                                                             className));

        synchronized (proxyInvocationHandlerPrepareForShutdownListenerList) {
            proxyInvocationHandlerPrepareForShutdownListenerList.add(listener);
            logger.debug("PrepareForShutdownListener implementation of {} has been registered", className);
        }
    }

    /**
     * Register to have HivemqMqttClientFactory.shutdown()
     * called at system shutdown.
     * @param listener ShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerHivemqMqttShutdownListener(ShutdownListener listener) throws IllegalArgumentException {
        final String className = listener.getClass().getName();
        if (!className.equals(HIVEMQ_MQTT_CLIENT_FACTORY_CLASS_NAME))
            throw new IllegalArgumentException(String.format(UNEXPECTED_CLASS_MESSAGE,
                                                             HIVEMQ_MQTT_CLIENT_FACTORY_CLASS_NAME,
                                                             className));

        hivemqMqttShutdownListener = listener;
    }

    /**
     * Register to have MessageTrackerForGracefulShutdown.shutdown()
     * called at system shutdown.
     * @param listener ShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerMessageTrackerShutdownListener(ShutdownListener listener) throws IllegalArgumentException {
        final String className = listener.getClass().getName();
        if (!className.equals(MESSAGE_TRACKER_FOR_GRACEFUL_SHUTDOWN_CLASS_NAME)) {
            throw new IllegalArgumentException(String.format(UNEXPECTED_CLASS_MESSAGE,
                                                             MESSAGE_TRACKER_FOR_GRACEFUL_SHUTDOWN_CLASS_NAME,
                                                             className));
        }

        messageTrackerShutdownListener = listener;
    }

    /**
     * Register to have ProxyInvocationHandler.shutdown()
     * called at system shutdown.
     * @param listener ShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerProxyInvocationHandlerShutdownListener(ShutdownListener listener) throws IllegalArgumentException {
        final String className = listener.getClass().getName();
        if (!className.contains(PROXY_INVOCATION_HANDLER_CLASS_NAME)) {
            throw new IllegalArgumentException(String.format(UNEXPECTED_CLASS_MESSAGE,
                                                             PROXY_INVOCATION_HANDLER_CLASS_NAME,
                                                             className));
        }

        synchronized (proxyInvocationHandlerShutdownListenerList) {
            proxyInvocationHandlerShutdownListenerList.add(listener);
            logger.debug("ShutdownListener implementation of {} has been registered", className);
        }
    }

    /**
     * Register to have the listener's shutdown method called at system shutdown
     * NOTE: no shutdown order is guaranteed registered using this method.
     * Listeners which should be invoked in specific order are to be registered
     * using dedicated register methods.
     * @param shutdownListener ShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerForShutdown(ShutdownListener shutdownListener) throws IllegalArgumentException {
        if (isDedicatedRegistrationRequired(shutdownListener)) {
            throw new IllegalArgumentException(DEDICATED_METHOD_REGISTRATION_REQUIRED_MESSAGE);
        }
        synchronized (shutdownListenerList) {
            shutdownListenerList.add(0, shutdownListener);
            logger.trace("#ShutdownListeners: {}", shutdownListenerList.size());
        }
    }

    /**
     * Register to have the listener's shutdown method called at system shutdown
     * as one of the last listeners. It is a partial ordering and ensures that this
     * listener's shutdown will be called after all listeners registered using
     * {@link #registerForShutdown(ShutdownListener)}.
     * NOTE: Listeners who manage some executor service should use this method.
     * @param shutdownListener ShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerToBeShutdownAsLast(ShutdownListener shutdownListener) throws IllegalArgumentException {
        if (isDedicatedRegistrationRequired(shutdownListener)) {
            throw new IllegalArgumentException(DEDICATED_METHOD_REGISTRATION_REQUIRED_MESSAGE);
        }
        synchronized (shutdownListenerList) {
            shutdownListenerList.add(shutdownListener);
        }
    }

    /**
     * Register to have the listener's prepareForShutdown method called at system
     * prepareForShutdown
     * NOTE: no shutdown order is guaranteed registered using this method.
     * Listeners which should be invoked in specific order are to be registered
     * using dedicated register methods.
     * @param prepareForShutdownListener ShutdownListener
     * @throws IllegalArgumentException when parent class of listener is of unsupported class
     */
    public void registerPrepareForShutdownListener(PrepareForShutdownListener prepareForShutdownListener) throws IllegalArgumentException {
        if (isDedicatedRegistrationRequired(prepareForShutdownListener)) {
            throw new IllegalArgumentException(DEDICATED_METHOD_REGISTRATION_REQUIRED_MESSAGE);
        }
        synchronized (prepareForShutdownListenerList) {
            prepareForShutdownListenerList.add(prepareForShutdownListener);
        }
    }

    private boolean isDedicatedRegistrationRequired(Object listener) {
        final String className = listener.getClass().getName();
        return className.equals(HIVEMQ_MQTT_CLIENT_FACTORY_CLASS_NAME)
                || className.equals(MESSAGE_TRACKER_FOR_GRACEFUL_SHUTDOWN_CLASS_NAME)
                || className.contains(PROXY_INVOCATION_HANDLER_CLASS_NAME);
    }

    /**
     * Will call {@link PrepareForShutdownListener#prepareForShutdown()} for each {@link #registerForShutdown(ShutdownListener) registered listener}
     * asynchronously, and waiting a total of five seconds for all to complete or will then timeout without waiting.
     */
    public void prepareForShutdown() throws InterruptedException {
        logger.debug("prepareForShutdown invoked");
        if (hivemqMqttPrepareForShutdownListener != null) {
            hivemqMqttPrepareForShutdownListener.prepareForShutdown();
        }

        Collection<CompletableFuture<Void>> proxyPrepareForShutdownFutures;
        synchronized (proxyInvocationHandlerPrepareForShutdownListenerList) {
            proxyPrepareForShutdownFutures = getPrepareForShutdownFutures(proxyInvocationHandlerPrepareForShutdownListenerList);
        }
        callPrepareForShutdownListeners(proxyPrepareForShutdownFutures);

        if (messageTrackerPrepareForShutdownListener != null) {
            messageTrackerPrepareForShutdownListener.prepareForShutdown();
        }

        synchronized (prepareForShutdownListenerList) {
            proxyPrepareForShutdownFutures = getPrepareForShutdownFutures(prepareForShutdownListenerList);
        }
        callPrepareForShutdownListeners(proxyPrepareForShutdownFutures);
    }

    private Collection<CompletableFuture<Void>> getPrepareForShutdownFutures(List<PrepareForShutdownListener> listeners) {
        return listeners.stream()
                        .map(listener -> CompletableFuture.runAsync(listener::prepareForShutdown))
                        .collect(Collectors.toList());
    }

    private void callPrepareForShutdownListeners(Collection<CompletableFuture<Void>> prepareForShutdownFutures) throws InterruptedException {
        try {
            CompletableFuture.allOf(prepareForShutdownFutures.toArray(new CompletableFuture[prepareForShutdownFutures.size()]))
                             .get(prepareForShutdownTimeoutSec, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            logger.error("Interrupted while waiting for joynr message queue to drain.");
            throw e;
        } catch (Exception e) {
            logger.error("Exception occurred while preparing shutdown.", e);
        }
    }

    /**
     * Calls {@link ShutdownListener#shutdown()} for each {@link #registerForShutdown(ShutdownListener) registered listener}
     * synchronously in turn.
     */
    public void shutdown() {
        if (hivemqMqttShutdownListener != null) {
            hivemqMqttShutdownListener.shutdown();
        }
        synchronized (proxyInvocationHandlerShutdownListenerList) {
            callShutdownListeners(proxyInvocationHandlerShutdownListenerList);
        }
        if (messageTrackerShutdownListener != null) {
            messageTrackerShutdownListener.shutdown();
        }
        synchronized (shutdownListenerList) {
            callShutdownListeners(shutdownListenerList);
        }
    }

    private void callShutdownListeners(List<ShutdownListener> listeners) {
        listeners.forEach(shutdownListener -> {
            logger.trace("Shutting down {}", shutdownListener);
            try {
                shutdownListener.shutdown();
            } catch (Exception e) {
                logger.error("Error shutting down {}:", shutdownListener, e);
            }
        });
    }

    public void unregister(PrepareForShutdownListener listener) {
        synchronized (prepareForShutdownListenerList) {
            prepareForShutdownListenerList.remove(listener);
            logger.trace("Removed ShutdownListener, #ShutdownListeners: {}", shutdownListenerList.size());
        }
    }
}
