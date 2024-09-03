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
package io.joynr.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.assistedinject.Assisted;

import io.joynr.Async;
import io.joynr.StatelessAsync;
import io.joynr.Sync;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.dispatcher.rpc.JoynrBroadcastSubscriptionInterface;
import io.joynr.dispatcher.rpc.JoynrSubscriptionInterface;
import io.joynr.dispatcher.rpc.annotation.FireAndForget;
import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.Invocation;
import io.joynr.proxy.invocation.MethodInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.proxy.invocation.StatelessAsyncMethodInvocation;
import io.joynr.proxy.invocation.SubscriptionInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.runtime.PrepareForShutdownListener;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.MethodMetaInformation;
import joynr.exceptions.ApplicationException;

public abstract class ProxyInvocationHandler implements InvocationHandler {
    private static final Logger logger = LoggerFactory.getLogger(ProxyInvocationHandler.class);
    protected Throwable throwable;
    protected final MessagingQos qosSettings;
    private ConnectorStatus connectorStatus;
    private boolean separateReplyReceiver;
    private Lock connectorStatusLock = new ReentrantLock();
    private Condition connectorFinished = connectorStatusLock.newCondition();
    private DiscoveryQos discoveryQos;
    protected ConnectorInvocationHandler connector;
    protected final String proxyParticipantId;
    private ConcurrentLinkedQueue<MethodInvocation<?>> queuedRpcList = new ConcurrentLinkedQueue<>();
    private ConcurrentLinkedQueue<SubscriptionAction> queuedSubscriptionInvocationList = new ConcurrentLinkedQueue<>();
    private ConcurrentLinkedQueue<UnsubscribeInvocation> queuedUnsubscribeInvocationList = new ConcurrentLinkedQueue<>();
    private ConcurrentLinkedQueue<StatelessAsyncMethodInvocation> queuedStatelessAsyncInvocationList = new ConcurrentLinkedQueue<>();
    private String interfaceName;
    private Set<String> domains;
    private final AtomicBoolean preparingForShutdown = new AtomicBoolean();
    protected String statelessAsyncParticipantId;
    protected ShutdownListener shutdownListener;
    protected PrepareForShutdownListener prepareForShutdownListener;

    // CHECKSTYLE:OFF
    public ProxyInvocationHandler(@Assisted("domains") Set<String> domains,
                                  @Assisted("interfaceName") String interfaceName,
                                  @Assisted("proxyParticipantId") String proxyParticipantId,
                                  @Assisted DiscoveryQos discoveryQos,
                                  @Assisted MessagingQos messagingQos,
                                  @Assisted Optional<StatelessAsyncCallback> statelessAsyncCallback,
                                  @Assisted boolean separateReplyReceiver,
                                  ShutdownNotifier shutdownNotifier,
                                  StatelessAsyncIdCalculator statelessAsyncIdCalculator) {
        // CHECKSTYLE:ON
        this.domains = (domains != null) ? new HashSet<>(domains) : null;
        this.proxyParticipantId = proxyParticipantId;
        this.interfaceName = interfaceName;
        this.discoveryQos = (discoveryQos != null) ? new DiscoveryQos(discoveryQos) : null;
        this.qosSettings = (messagingQos != null) ? new MessagingQos(messagingQos) : null;
        this.separateReplyReceiver = separateReplyReceiver;
        this.connectorStatus = ConnectorStatus.ConnectorNotAvailable;

        shutdownListener = new ShutdownListener() {
            @Override
            public void shutdown() {
                // No-op
            }
        };
        shutdownNotifier.registerProxyInvocationHandlerShutdownListener(shutdownListener);

        prepareForShutdownListener = new PrepareForShutdownListener() {
            @Override
            public void prepareForShutdown() {
                preparingForShutdown.set(true);
            }
        };
        shutdownNotifier.registerProxyInvocationHandlerPrepareForShutdownListener(prepareForShutdownListener);

        if (statelessAsyncCallback.isPresent()) {
            statelessAsyncParticipantId = statelessAsyncIdCalculator.calculateParticipantId(interfaceName,
                                                                                            statelessAsyncCallback.get());
        }
    }

    private interface ConnectorCaller {
        Object call(Method method, Object[] args) throws ApplicationException;
    }

    /**
     * executeSyncMethod is called whenever a method of the synchronous interface which is provided by the proxy is
     * called. The ProxyInvocationHandler will check the arbitration status before the call is delegated to the
     * connector. If the arbitration is still in progress the synchronous call will block until the arbitration was
     * successful or the timeout elapsed.
     *
     * @throws ApplicationException
     * @see java.lang.reflect.InvocationHandler#invoke(java.lang.Object, java.lang.reflect.Method, java.lang.Object[])
     */
    private Optional<Object> executeSyncMethod(Method method, Object[] args) throws ApplicationException {
        checkIfExecutionIsAllowed();
        return Optional.ofNullable(executeMethodWithCaller(method, args, new ConnectorCaller() {
            @Override
            public Object call(Method method, Object[] args) throws ApplicationException {
                return connector.executeSyncMethod(method, args);
            }
        }));
    }

    private Optional<Object> executeOneWayMethod(Method method, Object[] args) throws ApplicationException {
        Object result = executeMethodWithCaller(method, args, new ConnectorCaller() {
            @Override
            public Object call(Method method, Object[] args) {
                connector.executeOneWayMethod(method, args);
                return null;
            }
        });
        return Optional.ofNullable(result);
    }

    private Object executeMethodWithCaller(Method method,
                                           Object[] args,
                                           ConnectorCaller connectorCaller) throws ApplicationException {
        try {
            if (waitForConnectorFinished() && throwable == null) {
                if (connector == null) {
                    final String msg = String.format("Failed to execute sync method %s: Connector was null although arbitration finished successfully.",
                                                     method.getName());
                    logger.error(msg);
                    throw new IllegalStateException(msg);
                }
                return connectorCaller.call(method, args);
            }
        } catch (ApplicationException | JoynrRuntimeException e) {
            throw e;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new JoynrRuntimeException(e);
        } catch (Exception e) {
            throw new JoynrRuntimeException(e);
        }

        if (throwable != null) {
            logger.error("Failed to execute sync method {}: arbitration and Connector failed: domain: \"{}\" interface: \"{}\" qos: \"{}\": Arbitration could not be finished in time.\"",
                         method.getName(),
                         domains,
                         interfaceName,
                         discoveryQos,
                         throwable);
            if (throwable instanceof JoynrRuntimeException) {
                throw (JoynrRuntimeException) throwable;
            } else {
                throw new JoynrRuntimeException(throwable);
            }
        } else {
            logger.error("Failed to execute sync method {}: arbitration and Connector failed: domain: \"{}\" interface: \"{}\" qos: \"{}\": Arbitration could not be finished in time.\"",
                         method.getName(),
                         domains,
                         interfaceName,
                         discoveryQos);
            throw new DiscoveryException("Arbitration and Connector failed: domain: " + domains + " interface: "
                    + interfaceName + " qos: " + discoveryQos + ": Arbitration could not be finished in time.");
        }
    }

    /**
     * Checks the connector status before a method call is executed. Instantly returns True if the connector
     * already finished successfully, otherwise it will block until the ProxyInvocationHandler is notified
     * about a successful connection.
     *
     * @return True if the connector was finished successfully, False if the connector failed
     * @throws InterruptedException in case thread is interrupted
     */
    public boolean waitForConnectorFinished() throws InterruptedException {
        connectorStatusLock.lock();
        try {
            if (connectorStatus == ConnectorStatus.ConnectorSuccessful) {
                return true;
            }

            while (connectorStatus == ConnectorStatus.ConnectorNotAvailable) {
                connectorFinished.await();
            }
            return connectorStatus == ConnectorStatus.ConnectorSuccessful;

        } finally {
            connectorStatusLock.unlock();
        }

    }

    /**
     * Checks if the connector was set successfully. Returns immediately and does not block until the connector is
     * finished.
     *
     * @return true if a connector was successfully set.
     */
    public boolean isConnectorReady() {
        connectorStatusLock.lock();
        try {
            return connectorStatus == ConnectorStatus.ConnectorSuccessful;
        } finally {
            connectorStatusLock.unlock();
        }
    }

    private void sendQueuedSubscriptionInvocations() {
        while (true) {
            SubscriptionAction currentSubscriptionAction = queuedSubscriptionInvocationList.poll();
            if (currentSubscriptionAction == null) {
                return;
            }
            try {
                currentSubscriptionAction.subscribe();
            } catch (JoynrRuntimeException e) {
                currentSubscriptionAction.fail(e);
            } catch (Exception e) {
                currentSubscriptionAction.fail(new JoynrRuntimeException(e));
            }
        }
    }

    private void sendQueuedUnsubscribeInvocations() {
        while (true) {
            UnsubscribeInvocation unsubscribeInvocation = queuedUnsubscribeInvocationList.poll();
            if (unsubscribeInvocation == null) {
                return;
            }
            try {
                connector.executeSubscriptionMethod(unsubscribeInvocation);
            } catch (JoynrRuntimeException e) {
                unsubscribeInvocation.getFuture().onFailure(e);
            } catch (Exception e) {
                unsubscribeInvocation.getFuture().onFailure(new JoynrRuntimeException(e));
            }
        }
    }

    private void setFutureErrorState(Invocation<?> invocation, JoynrRuntimeException e) {
        invocation.getFuture().onFailure(e);
    }

    /**
     * Executes previously queued remote calls. This method is called when arbitration is completed.
     */
    private void sendQueuedInvocations() {
        while (true) {
            MethodInvocation<?> currentRPC = queuedRpcList.poll();
            if (currentRPC == null) {
                return;
            }

            try {
                connector.executeAsyncMethod(currentRPC.getProxy(),
                                             currentRPC.getMethod(),
                                             currentRPC.getArgs(),
                                             currentRPC.getFuture());
            } catch (JoynrRuntimeException e) {
                currentRPC.getFuture().onFailure(e);
            } catch (Exception e) {
                currentRPC.getFuture().onFailure(new JoynrRuntimeException(e));
            }

        }

    }

    private void sendQueuedStatelessAsyncInvocations() {
        while (true) {
            StatelessAsyncMethodInvocation invocation = queuedStatelessAsyncInvocationList.poll();
            if (invocation == null) {
                return;
            }
            try {
                connector.executeStatelessAsyncMethod(invocation.getMethod(), invocation.getArgs());
            } catch (Exception e) {
                logger.error("Unable to perform stateless async call {}:", invocation, e);
            }
        }
    }

    protected void setConnectorStatusSuccessAndSendQueuedRequests() {
        connectorStatusLock.lock();
        try {
            connectorStatus = ConnectorStatus.ConnectorSuccessful;
            connectorFinished.signalAll();

            if (connector != null) {
                sendQueuedInvocations();
                sendQueuedSubscriptionInvocations();
                sendQueuedUnsubscribeInvocations();
                sendQueuedStatelessAsyncInvocations();
            }
        } finally {
            connectorStatusLock.unlock();
        }
    }

    private Optional<Object> executeSubscriptionMethod(Object proxy, Method method, Object[] args) {
        Future<String> future = new Future<>();
        if (method.getName().startsWith("subscribeTo")) {
            checkIfExecutionIsAllowed();

            if (JoynrSubscriptionInterface.class.isAssignableFrom(method.getDeclaringClass())) {
                executeAttributeSubscriptionMethod(proxy, method, args, future);
            } else if (method.getAnnotation(JoynrRpcBroadcast.class) != null) {
                executeBroadcastSubscriptionMethod(proxy, method, args, future);
            } else if (method.getAnnotation(JoynrMulticast.class) != null) {
                executeMulticastSubscriptionMethod(proxy, method, args, future);
            } else {
                throw new JoynrRuntimeException("Method " + method
                        + " not declared in JoynrSubscriptionInterface or annotated with either @JoynrRpcBroadcast or @JoynrMulticast.");
            }
            return Optional.ofNullable(future);
        } else if (method.getName().startsWith("unsubscribeFrom")) {
            return Optional.ofNullable(unsubscribe(new UnsubscribeInvocation(method,
                                                                             args,
                                                                             future)).getSubscriptionId());
        } else {
            throw new JoynrIllegalStateException("Called unknown method in one of the subscription interfaces.");
        }
    }

    private void checkIfExecutionIsAllowed() {
        if (preparingForShutdown.get() && !separateReplyReceiver) {
            throw new JoynrIllegalStateException("Preparing for shutdown. "
                    + "Only stateless/unregister methods can be called.");
        }
    }

    private static abstract class SubscriptionAction {
        private Future<String> future;

        private SubscriptionAction(Future<String> future) {
            this.future = future;
        }

        protected abstract void subscribe();

        private void fail(JoynrException joynrException) {
            future.onFailure(joynrException);
        }
    }

    private void executeAttributeSubscriptionMethod(Object proxy, Method method, Object[] args, Future<String> future) {
        final AttributeSubscribeInvocation attributeSubscription = new AttributeSubscribeInvocation(method,
                                                                                                    args,
                                                                                                    future,
                                                                                                    proxy);
        queueOrExecuteSubscriptionInvocation(attributeSubscription, new SubscriptionAction(future) {
            @Override
            public void subscribe() {
                connector.executeSubscriptionMethod(attributeSubscription);
            }
        });

    }

    private void executeBroadcastSubscriptionMethod(Object proxy, Method method, Object[] args, Future<String> future) {
        final BroadcastSubscribeInvocation broadcastSubscription = new BroadcastSubscribeInvocation(method,
                                                                                                    args,
                                                                                                    future,
                                                                                                    proxy);
        queueOrExecuteSubscriptionInvocation(broadcastSubscription, new SubscriptionAction(future) {
            @Override
            public void subscribe() {
                connector.executeSubscriptionMethod(broadcastSubscription);
            }
        });
    }

    private void executeMulticastSubscriptionMethod(Object proxy, Method method, Object[] args, Future<String> future) {
        final MulticastSubscribeInvocation multicastSubscription = new MulticastSubscribeInvocation(method,
                                                                                                    args,
                                                                                                    future,
                                                                                                    proxy);
        queueOrExecuteSubscriptionInvocation(multicastSubscription, new SubscriptionAction(future) {
            @Override
            public void subscribe() {
                connector.executeSubscriptionMethod(multicastSubscription);
            }
        });
    }

    private void queueOrExecuteSubscriptionInvocation(SubscriptionInvocation subscriptionInvocation,
                                                      SubscriptionAction subscriptionMethodExecutor) {
        connectorStatusLock.lock();
        try {
            if (!isConnectorReady()) {
                queuedSubscriptionInvocationList.offer(subscriptionMethodExecutor);
                return;
            }
        } finally {
            connectorStatusLock.unlock();
        }
        try {
            subscriptionMethodExecutor.subscribe();
        } catch (JoynrRuntimeException e) {
            logger.error("Error executing subscription: {}:", subscriptionInvocation.getSubscriptionName(), e);
            setFutureErrorState(subscriptionInvocation, e);
        } catch (Exception e) {
            logger.error("Error executing subscription: {}:", subscriptionInvocation.getSubscriptionName(), e);
            setFutureErrorState(subscriptionInvocation, new JoynrRuntimeException(e));
        }
    }

    private <T> Object executeAsyncMethod(Object proxy, Method method, Object[] args) throws IllegalAccessException,
                                                                                      Exception {
        checkIfExecutionIsAllowed();
        @SuppressWarnings("unchecked")
        Future<T> future = (Future<T>) method.getReturnType().getConstructor().newInstance();

        connectorStatusLock.lock();
        try {
            if (!isConnectorReady()) {
                // waiting for arbitration -> queue invocation
                queuedRpcList.offer(new MethodInvocation<>(proxy, method, args, future));
                return future;
            }
        } finally {
            connectorStatusLock.unlock();
        }

        // arbitration already successfully finished -> send invocation
        return connector.executeAsyncMethod(proxy, method, args, future);
    }

    private void executeStatelessAsyncMethod(Method method, Object[] args) throws Exception {
        connectorStatusLock.lock();
        try {
            if (!isConnectorReady()) {
                queuedStatelessAsyncInvocationList.offer(new StatelessAsyncMethodInvocation(method, args));
                return;
            }
        } finally {
            connectorStatusLock.unlock();
        }
        connector.executeStatelessAsyncMethod(method, args);
    }

    private UnsubscribeInvocation unsubscribe(UnsubscribeInvocation unsubscribeInvocation) {
        connectorStatusLock.lock();
        try {
            if (!isConnectorReady()) {
                queuedUnsubscribeInvocationList.offer(unsubscribeInvocation);
                return unsubscribeInvocation;
            }
        } finally {
            connectorStatusLock.unlock();
        }

        try {
            connector.executeSubscriptionMethod(unsubscribeInvocation);
        } catch (JoynrRuntimeException e) {
            logger.error("Error executing unsubscription: {}:", unsubscribeInvocation.getSubscriptionId(), e);
            setFutureErrorState(unsubscribeInvocation, e);
        } catch (Exception e) {
            logger.error("Error executing unsubscription: {}:", unsubscribeInvocation.getSubscriptionId(), e);
            setFutureErrorState(unsubscribeInvocation, new JoynrRuntimeException(e));
        }
        return unsubscribeInvocation;
    }

    public Object invokeInternal(Object proxy, Method method, Object[] args) throws ApplicationException {
        logger.trace("Calling proxy.{}({}) on domain: {} and interface {}, proxy participant ID: {}",
                     method.getName(),
                     args,
                     domains,
                     interfaceName,
                     proxyParticipantId);
        Class<?> methodInterfaceClass = method.getDeclaringClass();
        try {
            if (JoynrSubscriptionInterface.class.isAssignableFrom(methodInterfaceClass)
                    || JoynrBroadcastSubscriptionInterface.class.isAssignableFrom(methodInterfaceClass)) {
                Optional<Object> result = executeSubscriptionMethod(proxy, method, args);
                return result.orElse(null);
            } else if (methodInterfaceClass.getAnnotation(FireAndForget.class) != null) {
                Optional<Object> result = executeOneWayMethod(method, args);
                return result.orElse(null);
            } else if (methodInterfaceClass.getAnnotation(Sync.class) != null) {
                Optional<Object> result = executeSyncMethod(method, args);
                return result.orElse(null);
            } else if (methodInterfaceClass.getAnnotation(Async.class) != null) {
                return executeAsyncMethod(proxy, method, args);
            } else if (methodInterfaceClass.getAnnotation(StatelessAsync.class) != null) {
                executeStatelessAsyncMethod(method, args);
                return null;
            } else {
                throw new JoynrIllegalStateException("Method is not part of sync, async or subscription interface");
            }
        } catch (JoynrRuntimeException | ApplicationException e) {
            throw e;
        } catch (Exception e) {
            throw new JoynrRuntimeException(e);
        }
    }

    public void abort(JoynrRuntimeException exception) {
        setThrowableForInvoke(exception);

        connectorStatusLock.lock();
        try {
            connectorStatus = ConnectorStatus.ConnectorFailed;
            connectorFinished.signalAll();
        } finally {
            connectorStatusLock.unlock();
        }

        for (MethodInvocation<?> invocation : queuedRpcList) {
            final MethodMetaInformation metaInfo = new MethodMetaInformation(invocation.getMethod());
            final int callbackIndex = metaInfo.getCallbackIndex();
            if (callbackIndex > -1) {
                ICallback callback = (ICallback) invocation.getArgs()[callbackIndex];
                try {
                    callback.onFailure(exception);
                } catch (final Exception callbackException) {
                    logger.error("Aborting call to method: {} but unable to call onError callback because of: ",
                                 invocation.getMethod().getName(),
                                 callbackException);
                }
            }

            invocation.getFuture().onFailure(exception);
        }

        for (Invocation<String> invocation : queuedUnsubscribeInvocationList) {
            invocation.getFuture().onFailure(exception);
        }

        for (SubscriptionAction subscriptionAction : queuedSubscriptionInvocationList) {
            subscriptionAction.fail(exception);
        }
        // TODO: abort StatelessAsyncMethodInvocations (queuedStatelessAsyncInvocationList) 
    }

    abstract void createConnector(ArbitrationResult result);

    /**
     * This method can be called to specify a throwable which will be thrown each time
     * {@link #invoke(Object, Method, Object[])} is called.
     *
     * @param throwable the throwable to be thrown when invoke is called.
     */
    void setThrowableForInvoke(Throwable throwable) {
        this.throwable = throwable;
    }

    /**
     * The InvocationHandler invoke method is mapped to the ProxyInvocationHandler.invokeInternal
     */
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if (throwable != null) {
            throw throwable;
        }
        try {
            return invokeInternal(proxy, method, args);
        } catch (Exception e) {
            if (this.throwable != null) {
                logger.trace("Exception caught: {} overridden by: {}", e.getMessage(), throwable.getMessage());
                throw throwable;
            } else {
                throw e;
            }
        }
    }

    abstract public void registerProxy(Object proxy);
}
