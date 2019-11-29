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
package io.joynr.jeeintegration;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import javax.ejb.Singleton;
import javax.inject.Inject;

import io.joynr.StatelessAsync;
import io.joynr.arbitration.DiscoveryQos;
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
 */
@Singleton
public class JeeJoynrServiceLocator implements ServiceLocator {

    private final JoynrIntegrationBean joynrIntegrationBean;

    @Inject
    public JeeJoynrServiceLocator(JoynrIntegrationBean joynrIntegrationBean) {
        this.joynrIntegrationBean = joynrIntegrationBean;
    }

    @Override
    public <I> I get(Class<I> serviceInterface, String domain) {
        return get(serviceInterface, domain, new MessagingQos(), new DiscoveryQos());
    }

    @Override
    public <I> I get(Class<I> serviceInterface, String domain, long ttl) {
        return get(serviceInterface, domain, new MessagingQos(ttl), new DiscoveryQos());
    }

    @Override
    public <I> I get(Class<I> serviceInterface, Set<String> domains) {
        return get(serviceInterface, domains, new MessagingQos(), new DiscoveryQos());
    }

    @Override
    public <I> I get(Class<I> serviceInterface, Set<String> domains, long ttl) {
        return get(serviceInterface, domains, new MessagingQos(ttl), new DiscoveryQos());
    }

    @Override
    public <I> I get(Class<I> serviceInterface, String domain, MessagingQos messagingQos, DiscoveryQos discoveryQos) {
        return get(serviceInterface, new HashSet<String>(Arrays.asList(domain)), messagingQos, discoveryQos);
    }

    @Override
    public <I> I get(Class<I> serviceInterface,
                     Set<String> domains,
                     MessagingQos messagingQos,
                     DiscoveryQos discoveryQos) {
        return get(serviceInterface, domains, messagingQos, discoveryQos, null);
    }

    @Override
    public <I> I get(Class<I> serviceInterface,
                     Set<String> domains,
                     MessagingQos messagingQos,
                     DiscoveryQos discoveryQos,
                     String useCase) {
        if (joynrIntegrationBean.getRuntime() == null) {
            throw new IllegalStateException("You can't get service proxies until the joynr runtime has been initialised.");
        }
        ProxyBuilder<I> proxyBuilder = joynrIntegrationBean.getRuntime()
                                                           .getProxyBuilder(domains, serviceInterface)
                                                           .setMessagingQos(messagingQos)
                                                           .setDiscoveryQos(discoveryQos);

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
        return proxyBuilder.build();
    }

    public class JeeJoynrServiceProxyBuilder<T> implements ServiceProxyBuilder<T> {

        private Class<T> serviceInterface;
        private Set<String> domains;
        private MessagingQos messagingQos = new MessagingQos();
        private DiscoveryQos discoveryQos = new DiscoveryQos();
        private String useCase;

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
            this.messagingQos = messagingQos;
            return this;
        }

        @Override
        public ServiceProxyBuilder<T> withDiscoveryQos(DiscoveryQos discoveryQos) {
            this.discoveryQos = discoveryQos;
            return this;
        }

        @Override
        public ServiceProxyBuilder<T> withUseCase(String useCase) {
            this.useCase = useCase;
            return this;
        }

        @Override
        public T build() {
            return get(serviceInterface, domains, messagingQos, discoveryQos, useCase);
        }
    }

    @Override
    public <I> ServiceProxyBuilder<I> builder(Class<I> serviceInterface, String... domains) {
        if (domains == null || domains.length == 0) {
            throw new JoynrRuntimeException("You must provide at least one domain.");
        }
        Set<String> domainSet = new HashSet<>(Arrays.asList(domains));
        return new JeeJoynrServiceProxyBuilder<>(serviceInterface, domainSet);
    }

    @Override
    public GuidedProxyBuilder getGuidedProxyBuilder(Class<?> interfaceClass, Set<String> domains) {
        return joynrIntegrationBean.getRuntime().getGuidedProxyBuilder(domains, interfaceClass);
    }

}
