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

import static java.lang.String.format;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import javax.ejb.Singleton;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.StatelessAsync;
import io.joynr.UsedBy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.util.AnnotationUtil;
import joynr.exceptions.ApplicationException;

/**
 * JEE integration joynr service locator which uses a joynr proxy to provide an implementation for a service interface.
 * The service interface is mapped to its joynr proxy, by expecting a {@link io.joynr.UsedBy} annotation
 * is attached in the class hierarchy of the service interface. With this annotation, the proxy is found by
 * the JeeJoynrServiceLocator.
 */
@Singleton
public class JeeJoynrServiceLocator implements ServiceLocator {

    private static final Logger LOG = LoggerFactory.getLogger(JeeJoynrServiceLocator.class);

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

    @SuppressWarnings("unchecked")
    @Override
    public <I> I get(Class<I> serviceInterface,
                     Set<String> domains,
                     MessagingQos messagingQos,
                     DiscoveryQos discoveryQos,
                     String useCase) {
        if (joynrIntegrationBean.getRuntime() == null) {
            throw new IllegalStateException("You can't get service proxies until the joynr runtime has been initialised.");
        }
        final Class<?> joynrProxyInterface = findJoynrProxyInterface(serviceInterface);
        ProxyBuilder<?> proxyBuilder = joynrIntegrationBean.getRuntime()
                                                           .getProxyBuilder(domains, joynrProxyInterface)
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

        final Object joynrProxy = proxyBuilder.build();
        return (I) Proxy.newProxyInstance(serviceInterface.getClassLoader(),
                                          new Class<?>[]{ serviceInterface },
                                          new InvocationHandler() {

                                              @Override
                                              public Object invoke(Object proxy,
                                                                   Method method,
                                                                   Object[] args) throws Throwable {
                                                  if (LOG.isTraceEnabled()) {
                                                      LOG.trace(format("Forwarding call to %s from service interface %s to joynr proxy %s",
                                                                       method,
                                                                       serviceInterface,
                                                                       joynrProxyInterface));
                                                  }
                                                  try {
                                                      return joynrProxy.getClass()
                                                                       .getMethod(method.getName(),
                                                                                  method.getParameterTypes())
                                                                       .invoke(joynrProxy, args);
                                                  } catch (InvocationTargetException e) {
                                                      if (e.getTargetException() instanceof JoynrRuntimeException) {
                                                          throw e.getTargetException();
                                                      } else if (e.getTargetException() instanceof ApplicationException) {
                                                          throw e.getTargetException();
                                                      }
                                                      throw e;
                                                  }
                                              }
                                          });
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

    private <I> Class<?> findJoynrProxyInterface(Class<I> serviceInterface) {
        UsedBy usedByAnnotation = AnnotationUtil.getAnnotation(serviceInterface, UsedBy.class);
        if (usedByAnnotation == null) {
            throw new IllegalArgumentException(format("Unable to find suitable joynr proxy for interface %s",
                                                      serviceInterface));
        }
        return usedByAnnotation.value();
    }

}
