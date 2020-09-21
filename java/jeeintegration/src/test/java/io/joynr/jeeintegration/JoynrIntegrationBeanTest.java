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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.name.Names;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys;
import io.joynr.jeeintegration.api.ProviderDomain;
import io.joynr.jeeintegration.api.ProviderRegistrationSettingsFactory;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Future;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ProviderRegistrar;
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectorySync;
import joynr.jeeintegration.servicelocator.MyServiceProvider;
import joynr.jeeintegration.servicelocator.MyServiceSync;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;

/**
 * Unit tests for the {@link JoynrIntegrationBean}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JoynrIntegrationBeanTest {

    private static final String LOCAL_DOMAIN = "local.domain";
    private static final String MY_CUSTOM_DOMAIN = "my.custom.domain";
    private static final String SETTINGS_DOMAIN = "settings.domain";

    private static final int REGISTRATION_RETRY_INTERVAL = 500;
    private static final int REGISTRATION_RETRIES = 2;

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    private static class MyServiceBean implements MyServiceSync {
        @Override
        public String callMe(String parameterOne) {
            return null;
        }

        @Override
        public void callMeWithException() throws ApplicationException {
            throw new ApplicationException(null);
        }
    }

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    @ProviderDomain(MY_CUSTOM_DOMAIN)
    private static class CustomDomainMyServiceBean implements MyServiceSync {
        @Override
        public String callMe(String parameterOne) {
            return null;
        }

        @Override
        public void callMeWithException() throws ApplicationException {
            throw new ApplicationException(null);
        }
    }

    @ServiceProvider(serviceInterface = GlobalCapabilitiesDirectorySync.class)
    private static class GcdBean implements GlobalCapabilitiesDirectorySync {

        @Override
        public void add(GlobalDiscoveryEntry[] globalDiscoveryEntries) {
        }

        @Override
        public void add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        }

        @Override
        public void add(GlobalDiscoveryEntry globalDiscoveryEntry, String[] gbids) throws ApplicationException {
        }

        @Override
        public GlobalDiscoveryEntry[] lookup(String[] domains, String interfaceName) {
            return null;
        }

        @Override
        public GlobalDiscoveryEntry[] lookup(String[] domains,
                                             String interfaceName,
                                             String[] gbids) throws ApplicationException {
            return null;
        }

        @Override
        public GlobalDiscoveryEntry lookup(String participantId) {
            return null;
        }

        @Override
        public GlobalDiscoveryEntry lookup(String participantId, String[] gbids) throws ApplicationException {
            return null;
        }

        @Override
        public void remove(String[] participantIds) {
        }

        @Override
        public void remove(String participantId) {
        }

        @Override
        public void remove(String participantId, String[] gbids) throws ApplicationException {
        }

        @Override
        public void removeStale(String clusterControllerId, Long maxLastSeenDateMs) {
        }

        @Override
        public void touch(String clusterControllerId) {
        }

        @Override
        public void touch(String clusterControllerId, String[] participantIds) {
        }
    }

    private static class MyProviderSettingsFactory implements ProviderRegistrationSettingsFactory {
        @Override
        public ProviderQos createProviderQos() {
            ProviderQos providerQos = new ProviderQos();
            providerQos.setPriority(100L);
            return providerQos;
        }

        @Override
        public String[] createGbids() {
            return new String[]{ "gbid1", "gbid2" };
        }

        @Override
        public String createDomain() {
            String domain = SETTINGS_DOMAIN;
            return domain;
        }

        @Override
        public boolean providesFor(Class<?> serviceInterface) {
            return MyServiceSync.class.isAssignableFrom(serviceInterface);
        }
    }

    @Mock
    private BeanManager beanManager;

    @Mock
    private JoynrRuntimeFactory joynrRuntimeFactory;

    @Mock
    private JoynrRuntime joynrRuntime;

    @Mock
    private ProviderRegistrar providerRegistrar;

    @Mock
    private ServiceProviderDiscovery serviceProviderDiscovery;

    @Mock
    private CallbackHandlerDiscovery callbackHandlerDiscovery;

    @Mock
    private Future<Void> mockVoidFuture;

    private JoynrIntegrationBean subject;

    private Set<Bean<?>> serviceProviderBeans;

    @Before
    public void setup() {
        serviceProviderBeans = new HashSet<>();
        @SuppressWarnings("rawtypes")
        Bean bean = mock(Bean.class);
        when(bean.getBeanClass()).thenReturn(MyServiceBean.class);
        serviceProviderBeans.add(bean);
        when(joynrRuntimeFactory.getInjector()).thenReturn(Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Integer.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES))
                                   .toInstance(REGISTRATION_RETRIES);
                bind(Integer.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS))
                                   .toInstance(REGISTRATION_RETRY_INTERVAL);
                bind(Boolean.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_AWAIT_REGISTRATION))
                                   .toInstance(true);
            }
        }));
        when(joynrRuntimeFactory.create(any())).thenReturn(joynrRuntime);
        when(joynrRuntimeFactory.getLocalDomain()).thenReturn(LOCAL_DOMAIN);
        when(joynrRuntime.getProviderRegistrar(any(), any())).thenReturn(providerRegistrar);
        when(providerRegistrar.withProviderQos(any())).thenReturn(providerRegistrar);
        when(providerRegistrar.withGbids(any())).thenReturn(providerRegistrar);
        when(providerRegistrar.awaitGlobalRegistration()).thenReturn(providerRegistrar);
        doReturn(mockVoidFuture).when(providerRegistrar).register();

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                return MyServiceProvider.class;
            }
        }).when(serviceProviderDiscovery).getProviderInterfaceFor(eq(MyServiceSync.class));

        subject = new JoynrIntegrationBean(beanManager,
                                           joynrRuntimeFactory,
                                           serviceProviderDiscovery,
                                           callbackHandlerDiscovery);
    }

    @Test
    public void testInitialise() {
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(new HashSet<>());

        subject.initialise();

        verify(joynrRuntimeFactory).create(new HashSet<>());
        verify(serviceProviderDiscovery).findServiceProviderBeans();
    }

    @Test
    public void testRegisterProviderWithLocalDomain() throws Exception {
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        subject.initialise();
        verify(joynrRuntime).getProviderRegistrar(eq(LOCAL_DOMAIN), any());
        verify(providerRegistrar).awaitGlobalRegistration();
        verify(providerRegistrar).register();
        verify(mockVoidFuture).get(MessagingQos.DEFAULT_TTL + 11000);
    }

    @Test
    public void testRegisterProviderWithDifferentDomain() {
        serviceProviderBeans = new HashSet<>();
        @SuppressWarnings("rawtypes")
        Bean bean = mock(Bean.class);
        when(bean.getBeanClass()).thenReturn(CustomDomainMyServiceBean.class);
        serviceProviderBeans.add(bean);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        subject.initialise();
        verify(joynrRuntime).getProviderRegistrar(eq(MY_CUSTOM_DOMAIN), any());
        ProviderQos expectedProviderQos = new ProviderQos();
        verify(providerRegistrar).withProviderQos(expectedProviderQos);
        String[] expectedGbids = {};
        verify(providerRegistrar).withGbids(expectedGbids);
        verify(providerRegistrar).awaitGlobalRegistration();
        verify(providerRegistrar).register();
    }

    @Test
    public void testRegisterProviderUsingSettingsFromFactory() {
        // given we have a bean implementing a joynr provider...
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        // ...and a factory for customized provider registration settings
        MyProviderSettingsFactory settingsFactory = new MyProviderSettingsFactory();

        //... and the bean manager mock returns this factory wrapped in a bean
        doAnswer(new Answer<Object>() {
            @SuppressWarnings("unchecked")
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("rawtypes")
                Bean factoryAsBean = mock(Bean.class);
                when(factoryAsBean.getBeanClass()).thenReturn(MyProviderSettingsFactory.class);
                when(factoryAsBean.create(null)).thenReturn(settingsFactory);

                Set<Bean<?>> providerSettingsFactoryBeans = new HashSet<>();
                providerSettingsFactoryBeans.add(factoryAsBean);
                return providerSettingsFactoryBeans;
            }
        }).when(beanManager).getBeans(eq(ProviderRegistrationSettingsFactory.class), any());

        // when we initialize the subject (and register providers)
        subject.initialise();

        // then the runtime is called with the correct parameters from the factory
        ProviderQos expectedProviderQos = new ProviderQos();
        expectedProviderQos.setPriority(100L);
        String[] expectedGbids = new String[]{ "gbid1", "gbid2" };
        String expectedDomain = SETTINGS_DOMAIN;
        verify(joynrRuntime).getProviderRegistrar(eq(expectedDomain), any());
        verify(providerRegistrar).withProviderQos(expectedProviderQos);
        verify(providerRegistrar).withGbids(expectedGbids);
        verify(providerRegistrar).awaitGlobalRegistration();
        verify(providerRegistrar).register();
    }

    @Test
    public void testRegisterProviderWithRetry_oneRetryUntilSuccess() throws Exception {
        Semaphore semaphore = new Semaphore(0);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        doReturn(mockVoidFuture).when(providerRegistrar).register();
        doAnswer(new Answer<Void>() {
            private boolean firstInvocation = true;

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                semaphore.release();
                if (firstInvocation) {
                    firstInvocation = false;
                    throw new JoynrRuntimeException();
                }
                return null;
            }
        }).when(mockVoidFuture).get(anyLong());

        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                subject.initialise();
            }
        });
        t.start();

        assertTrue(semaphore.tryAcquire(REGISTRATION_RETRY_INTERVAL * 2, TimeUnit.MILLISECONDS));
        long firstTry = System.currentTimeMillis();
        Thread.sleep(REGISTRATION_RETRY_INTERVAL / 2);
        verify(joynrRuntime, times(1)).getProviderRegistrar(eq(LOCAL_DOMAIN), any());
        ProviderQos expectedProviderQos = new ProviderQos();
        verify(providerRegistrar, times(1)).withProviderQos(expectedProviderQos);
        String[] expectedGbids = {};
        verify(providerRegistrar, times(1)).withGbids(expectedGbids);
        verify(providerRegistrar, times(1)).awaitGlobalRegistration();
        verify(providerRegistrar, times(1)).register();

        assertTrue(semaphore.tryAcquire(REGISTRATION_RETRY_INTERVAL, TimeUnit.MILLISECONDS));
        long secondTry = System.currentTimeMillis();
        verify(providerRegistrar, times(2)).register();

        long delta = secondTry - firstTry;
        assertTrue(delta >= REGISTRATION_RETRY_INTERVAL && delta < REGISTRATION_RETRY_INTERVAL * 1.5);

        t.join();
        verify(joynrRuntime, times(1)).getProviderRegistrar(eq(LOCAL_DOMAIN), any());
        verify(providerRegistrar, times(1)).withProviderQos(expectedProviderQos);
        verify(providerRegistrar, times(1)).withGbids(expectedGbids);
        verify(providerRegistrar, times(1)).awaitGlobalRegistration();
        verify(providerRegistrar, times(2)).register();
        verify(mockVoidFuture, times(2)).get(MessagingQos.DEFAULT_TTL + 11000);
    }

    @Test
    public void testRegisterProviderWithRetry_noSuccess() throws Exception {
        Semaphore semaphore = new Semaphore(0);
        Semaphore semaphore2 = new Semaphore(0);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        JoynrRuntimeException exceptionFromFuture = new JoynrRuntimeException("exception from future");
        JoynrRuntimeException expectedException = new JoynrRuntimeException("Provider registration failed for bean "
                + serviceProviderBeans.iterator().next(), exceptionFromFuture);

        doReturn(mockVoidFuture).when(providerRegistrar).register();
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                semaphore.release();
                throw new JoynrRuntimeException();
            }
        }).when(mockVoidFuture).get(anyLong());

        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    subject.initialise();
                } catch (JoynrRuntimeException e) {
                    assertEquals(expectedException, e);
                    semaphore2.release();
                }
            }
        });
        t.start();

        assertTrue(semaphore.tryAcquire(REGISTRATION_RETRY_INTERVAL * 2, TimeUnit.MILLISECONDS));
        long firstTry = System.currentTimeMillis();
        Thread.sleep(REGISTRATION_RETRY_INTERVAL / 2);
        verify(providerRegistrar, times(1)).register();

        assertTrue(semaphore2.tryAcquire(REGISTRATION_RETRY_INTERVAL * (REGISTRATION_RETRIES + 1),
                                         TimeUnit.MILLISECONDS));
        long end = System.currentTimeMillis();
        verify(providerRegistrar, times(3)).register();

        long delta = end - firstTry;
        assertTrue(delta >= REGISTRATION_RETRY_INTERVAL * 2 && delta < REGISTRATION_RETRY_INTERVAL * 2.5);

        t.join();
        verify(joynrRuntime, times(1)).getProviderRegistrar(eq(LOCAL_DOMAIN), any());
        ProviderQos expectedProviderQos = new ProviderQos();
        verify(providerRegistrar, times(1)).withProviderQos(expectedProviderQos);
        String[] expectedGbids = {};
        verify(providerRegistrar, times(1)).withGbids(expectedGbids);
        verify(providerRegistrar, times(1)).awaitGlobalRegistration();
        verify(providerRegistrar, times(3)).register();
        verify(mockVoidFuture, times(3)).get(MessagingQos.DEFAULT_TTL + 11000);
    }

    @Test
    public void testRegisterGcd_noAwaitGlobalRegistration() throws Exception {
        serviceProviderBeans = new HashSet<>();
        @SuppressWarnings("rawtypes")
        Bean bean = mock(Bean.class);
        when(bean.getBeanClass()).thenReturn(GcdBean.class);
        serviceProviderBeans.add(bean);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);
        doReturn(GlobalCapabilitiesDirectoryProvider.class).when(serviceProviderDiscovery)
                                                           .getProviderInterfaceFor(eq(GlobalCapabilitiesDirectorySync.class));

        subject.initialise();
        verify(joynrRuntime).getProviderRegistrar(eq(LOCAL_DOMAIN), any());
        ProviderQos expectedProviderQos = new ProviderQos();
        verify(providerRegistrar, times(1)).withProviderQos(expectedProviderQos);
        String[] expectedGbids = {};
        verify(providerRegistrar, times(1)).withGbids(expectedGbids);
        verify(providerRegistrar, times(0)).awaitGlobalRegistration();
        verify(providerRegistrar, times(1)).register();
        verify(mockVoidFuture, times(1)).get(MessagingQos.DEFAULT_TTL + 11000);
    }

    @Test
    public void testRegisterWithAwaitGlobalRegistrationDisabled_singleProvider() throws Exception {
        when(joynrRuntimeFactory.getInjector()).thenReturn(Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Integer.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES))
                                   .toInstance(REGISTRATION_RETRIES);
                bind(Integer.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS))
                                   .toInstance(REGISTRATION_RETRY_INTERVAL);
                bind(Boolean.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_AWAIT_REGISTRATION))
                                   .toInstance(false);
            }
        }));

        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        doReturn(mockVoidFuture).when(providerRegistrar).register();

        subject.initialise();

        ProviderQos expectedProviderQos = new ProviderQos();
        String[] expectedGbids = {};
        verify(joynrRuntime, times(1)).getProviderRegistrar(eq(LOCAL_DOMAIN), any());
        verify(providerRegistrar, times(1)).withProviderQos(expectedProviderQos);
        verify(providerRegistrar, times(1)).withGbids(expectedGbids);
        verify(providerRegistrar, times(0)).awaitGlobalRegistration();
        verify(providerRegistrar, times(1)).register();
        verify(mockVoidFuture, times(0)).get(anyLong());
    }

    @Test
    public void testRegisterWithAwaitGlobalRegistrationDisabled_twoProviders() throws Exception {
        @SuppressWarnings("rawtypes")
        Bean bean = mock(Bean.class);
        when(bean.getBeanClass()).thenReturn(CustomDomainMyServiceBean.class);
        serviceProviderBeans.add(bean);

        doReturn(Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Integer.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES))
                                   .toInstance(REGISTRATION_RETRIES);
                bind(Integer.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS))
                                   .toInstance(REGISTRATION_RETRY_INTERVAL);
                bind(Boolean.class).annotatedWith(Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_AWAIT_REGISTRATION))
                                   .toInstance(false);
            }
        })).when(joynrRuntimeFactory).getInjector();
        doReturn(serviceProviderBeans).when(serviceProviderDiscovery).findServiceProviderBeans();
        doReturn(mockVoidFuture).when(providerRegistrar).register();

        subject.initialise();

        ProviderQos expectedProviderQos = new ProviderQos();
        String[] expectedGbids = {};
        verify(joynrRuntime, times(1)).getProviderRegistrar(eq(LOCAL_DOMAIN), any());
        verify(joynrRuntime, times(1)).getProviderRegistrar(eq(MY_CUSTOM_DOMAIN), any());
        verify(providerRegistrar, times(2)).withProviderQos(expectedProviderQos);
        verify(providerRegistrar, times(2)).withGbids(expectedGbids);
        verify(providerRegistrar, times(0)).awaitGlobalRegistration();
        verify(providerRegistrar, times(2)).register();
        verify(mockVoidFuture, times(0)).get(anyLong());
    }
}
