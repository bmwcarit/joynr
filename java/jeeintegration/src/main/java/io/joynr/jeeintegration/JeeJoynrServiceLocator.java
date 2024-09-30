/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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
package io.joynr.jeeintegration;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CompletableFuture;

import jakarta.ejb.Singleton;
import jakarta.inject.Inject;

import io.joynr.StatelessAsync;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.ProxyBuilder;

/**
 * JEE integration joynr service locator which uses a joynr proxy to provide an implementation for a service interface.
 * The service interface is mapped to its joynr proxy, by expecting a {@link io.joynr.UsedBy} annotation
 * is attached in the class hierarchy of the service interface. With this annotation, the proxy is found by
 * the JeeJoynrServiceLocator.
 * <p>
 * This class provides some helper methods for common use cases for obtaining a service proxy in the form of the
 * various <code>get</code> methods. In order to have access to all features of the joynr proxy builder, such as
 * specifying the stateless async use case or getting a <code>CompletableFuture</code> for the service proxy, use
 * the {@link io.joynr.jeeintegration.api.ServiceLocator.ServiceProxyBuilder} which can be obtained by calling
 * {@link #builder(Class, String...)} instead.
 * </p>
 * <p>
 * Note that unless you use {@link ServiceProxyBuilder#useFuture()}, then the service proxy returned might not initially
 * be connected to the desired provider while arbitration is ongoing. Any calls you make the service proxy in this state
 * will be queued until such a time as the provider becomes available. If you want more fine-grained control over how
 * to handle the provider not being available, becoming available or the arbitration failing, then call the
 * <code>useFuture()</code> method before calling the <code>build()</code> method and add the relevant listeners to
 * the resulting completable future.
 * </p>
 */
@Singleton
public class JeeJoynrServiceLocator implements ServiceLocator {

    private final JoynrIntegrationBean joynrIntegrationBean;

    @Inject
    public JeeJoynrServiceLocator(JoynrIntegrationBean joynrIntegrationBean) {
        this.joynrIntegrationBean = joynrIntegrationBean;
    }

    @Override
    @Deprecated
    public <I> I get(Class<I> serviceInterface, String domain) {
        return get(serviceInterface, domain, new MessagingQos(), new DiscoveryQos());
    }

    @Override
    @Deprecated
    public <I> I get(Class<I> serviceInterface, String domain, long ttl) {
        return get(serviceInterface, domain, new MessagingQos(ttl), new DiscoveryQos());
    }

    @Override
    @Deprecated
    public <I> I get(Class<I> serviceInterface, Set<String> domains) {
        return get(serviceInterface, domains, new MessagingQos(), new DiscoveryQos());
    }

    @Override
    @Deprecated
    public <I> I get(Class<I> serviceInterface, Set<String> domains, long ttl) {
        return get(serviceInterface, domains, new MessagingQos(ttl), new DiscoveryQos());
    }

    @Override
    @Deprecated
    public <I> I get(Class<I> serviceInterface, String domain, MessagingQos messagingQos, DiscoveryQos discoveryQos) {
        return get(serviceInterface, new HashSet<String>(Arrays.asList(domain)), messagingQos, discoveryQos);
    }

    @Override
    @Deprecated
    public <I> I get(Class<I> serviceInterface,
                     Set<String> domains,
                     MessagingQos messagingQos,
                     DiscoveryQos discoveryQos) {
        return get(serviceInterface, domains, messagingQos, discoveryQos, null);
    }

    @Override
    @Deprecated
    public <I> I get(Class<I> serviceInterface,
                     Set<String> domains,
                     MessagingQos messagingQos,
                     DiscoveryQos discoveryQos,
                     String useCase) {
        return get(serviceInterface, domains, messagingQos, discoveryQos, useCase, null, null);
    }

    private <I> I get(Class<I> serviceInterface,
                      Set<String> domains,
                      MessagingQos messagingQos,
                      DiscoveryQos discoveryQos,
                      String useCase,
                      String[] gbids,
                      ProxyBuilder.ProxyCreatedCallback<I> proxyCreatedCallback) {
        if (joynrIntegrationBean.getRuntime() == null) {
            throw new IllegalStateException("You can't get service proxies until the joynr runtime has been initialised.");
        }
        ProxyBuilder<I> proxyBuilder = joynrIntegrationBean.getRuntime()
                                                           .getProxyBuilder(domains, serviceInterface)
                                                           .setMessagingQos(messagingQos)
                                                           .setDiscoveryQos(discoveryQos);
        if (gbids != null) {
            proxyBuilder.setGbids(gbids);
        }

        if (useCase != null) {
            if (serviceInterface.getAnnotation(StatelessAsync.class) == null) {
                throw new IllegalArgumentException("Service interface " + serviceInterface
                        + " is not @StatelessAsync, but you provided a use case for a callback handler.");
            }
            proxyBuilder.setStatelessAsyncCallbackUseCase(useCase);
        } else if (serviceInterface.getAnnotation(StatelessAsync.class) != null) {
            throw new IllegalArgumentException("Service interface " + serviceInterface
                    + " is @StatelessAsync, but you failed to provide a use case.");
        }
        if (proxyCreatedCallback != null) {
            return proxyBuilder.build(proxyCreatedCallback);
        }
        return proxyBuilder.build();
    }

    @Override
    public <I> ServiceProxyBuilder<I> builder(Class<I> serviceInterface, String... domains) {
        if (domains == null || domains.length == 0) {
            throw new JoynrRuntimeException("You must provide at least one domain.");
        }
        Set<String> domainSet = new HashSet<>(Arrays.asList(domains));
        return new JeeJoynrServiceProxyBuilder<>(serviceInterface, domainSet);
    }

    public class JeeJoynrServiceProxyBuilder<T> implements ServiceProxyBuilder<T> {

        private Class<T> serviceInterface;
        private Set<String> domains;
        private MessagingQos messagingQos = new MessagingQos();
        private DiscoveryQos discoveryQos = new DiscoveryQos();
        private String useCase;
        private String[] gbids;
        private ProxyBuilder.ProxyCreatedCallback<T> callback;

        private JeeJoynrServiceProxyBuilder(Class<T> serviceInterface, Set<String> domains) {
            this.serviceInterface = serviceInterface;
            this.domains = domains;
        }

        @Override
        public ServiceProxyBuilder<T> withTtl(long ttl) {
            messagingQos.setTtl_ms(ttl);
            return this;
        }

        @Override
        public ServiceProxyBuilder<T> withMessagingQos(MessagingQos messagingQos) {
            this.messagingQos = (messagingQos != null) ? new MessagingQos(messagingQos) : null;
            return this;
        }

        @Override
        public ServiceProxyBuilder<T> withDiscoveryQos(DiscoveryQos discoveryQos) {
            this.discoveryQos = (discoveryQos != null) ? new DiscoveryQos(discoveryQos) : null;
            return this;
        }

        @Override
        public ServiceProxyBuilder<T> withUseCase(String useCase) {
            this.useCase = useCase;
            return this;
        }

        @Override
        public ServiceProxyBuilder<T> withGbids(String[] gbids) {
            if (gbids == null || gbids.length == 0) {
                throw new IllegalArgumentException("gbids array must not be null or empty");
            }
            this.gbids = gbids.clone();
            return this;
        }

        @Override
        public ServiceProxyBuilder<T> withCallback(ProxyBuilder.ProxyCreatedCallback<T> callback) {
            this.callback = callback;
            return this;
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<T>> useFuture() {
            return new JeeJoynrServiceFutureProxyBuilder<>(this);
        }

        @Override
        public T build() {
            return get(serviceInterface, domains, messagingQos, discoveryQos, useCase, gbids, callback);
        }
    }

    public class JeeJoynrServiceFutureProxyBuilder<T> implements ServiceProxyBuilder<CompletableFuture<T>> {

        private JeeJoynrServiceProxyBuilder<T> wrappedBuilder;

        private JeeJoynrServiceFutureProxyBuilder(JeeJoynrServiceProxyBuilder<T> wrappedBuilder) {
            this.wrappedBuilder = wrappedBuilder;
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<T>> withTtl(long ttl) {
            wrappedBuilder.withTtl(ttl);
            return this;
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<T>> withMessagingQos(MessagingQos messagingQos) {
            wrappedBuilder.withMessagingQos(messagingQos);
            return this;
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<T>> withDiscoveryQos(DiscoveryQos discoveryQos) {
            wrappedBuilder.withDiscoveryQos(discoveryQos);
            return this;
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<T>> withUseCase(String useCase) {
            wrappedBuilder.withUseCase(useCase);
            return this;
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<T>> withGbids(String[] gbids) {
            wrappedBuilder.withGbids(gbids);
            return this;
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<T>> withCallback(ProxyBuilder.ProxyCreatedCallback<CompletableFuture<T>> callback) {
            throw new IllegalStateException("The builder is using a future. Attach any callback you want to use in addition to the future before calling useFuture().");
        }

        @Override
        public ServiceProxyBuilder<CompletableFuture<CompletableFuture<T>>> useFuture() {
            throw new IllegalStateException("The builder will already provide a future. Ensure that you only call useFuture() once.");
        }

        @Override
        public CompletableFuture<T> build() {
            CompletableFuture<T> future = new CompletableFuture<>();
            get(wrappedBuilder.serviceInterface,
                wrappedBuilder.domains,
                wrappedBuilder.messagingQos,
                wrappedBuilder.discoveryQos,
                wrappedBuilder.useCase,
                wrappedBuilder.gbids,
                new ProxyBuilder.ProxyCreatedCallback<T>() {
                    @Override
                    public void onProxyCreationFinished(T result) {
                        future.complete(result);
                        if (wrappedBuilder.callback != null) {
                            wrappedBuilder.callback.onProxyCreationFinished(result);
                        }
                    }

                    @Override
                    public void onProxyCreationError(JoynrRuntimeException error) {
                        future.completeExceptionally(error);
                        if (wrappedBuilder.callback != null) {
                            wrappedBuilder.callback.onProxyCreationError(error);
                        }
                    }

                    @Override
                    public void onProxyCreationError(DiscoveryException error) {
                        future.completeExceptionally(error);
                        if (wrappedBuilder.callback != null) {
                            wrappedBuilder.callback.onProxyCreationError(error);
                        }
                    }
                });
            return future;
        }
    }

    @Override
    public GuidedProxyBuilder getGuidedProxyBuilder(Class<?> interfaceClass, Set<String> domains) {
        return joynrIntegrationBean.getRuntime().getGuidedProxyBuilder(domains, interfaceClass);
    }

}
