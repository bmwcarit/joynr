package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.when;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.dispatcher.MessagingEndpointDirectory;
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderDefaultImpl;
import io.joynr.runtime.JoynrRuntime;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Properties;

import joynr.types.CapabilityInformation;
import joynr.types.CustomParameter;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.Spy;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.common.collect.Lists;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.name.Names;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryTest {
    @Mock
    JoynrRuntime runtime;

    private LocalCapabilitiesDirectory localCapabilitiesDirectory;

    @Mock
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;

    @Mock
    private MessagingEndpointDirectory messagingEndpointDirectoryMock;

    @Spy
    protected CapabilitiesStore localCapabilitiesStoreSpy = new CapabilitiesStoreImpl();

    @Spy
    protected CapabilitiesStore globalCapabilitiesCacheSpy = new CapabilitiesStoreImpl();

    private CapabilityEntry capabilityEntry;
    private CapabilityInformation capabilityInformation;

    public interface TestInterface extends JoynrInterface {
        public static final String INTERFACE_NAME = "interfaceName";
    }

    private Injector injector;

    @SuppressWarnings("unchecked")
    @Before
    public void setUp() {

        String channelId = "testChannelId";

        Answer<Void> answer = new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                return null;
            }

        };
        Mockito.doAnswer(answer).when(globalCapabilitiesClient).add(Mockito.any(Callback.class),
                                                                    Mockito.any(CapabilityInformation.class));

        Module module = new AbstractModule() {

            @Override
            protected void configure() {
                bind(MessagingEndpointDirectory.class).toInstance(messagingEndpointDirectoryMock);
                bind(GlobalCapabilitiesDirectoryClient.class).toInstance(globalCapabilitiesClient);
                bind(String.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN))
                                  .toInstance("com.bmw");
                bind(String.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID))
                                  .toInstance("channelUrlDir_participantId");
                bind(String.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID))
                                  .toInstance("dirchannelId");
                bind(String.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID))
                                  .toInstance("capDir_participantId");
                bind(String.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID))
                                  .toInstance("dirchannelId");
                bind(String.class).annotatedWith(Names.named(MessagingPropertyKeys.CHANNELID)).toInstance("channelId");
                bind(Properties.class).annotatedWith(Names.named(MessagingPropertyKeys.JOYNR_PROPERTIES))
                                      .toInstance(new Properties());
            }
        };

        injector = Guice.createInjector(module);
        localCapabilitiesDirectory = new LocalCapabilitiesDirectoryImpl(localCapabilitiesStoreSpy,
                                                                        globalCapabilitiesCacheSpy);

        injector.injectMembers(localCapabilitiesDirectory);

        when(runtime.getProxyBuilder(any(String.class), any(Class.class))).thenReturn(new ProxyBuilderDefaultImpl<TestInterface>(localCapabilitiesDirectory,
                                                                                                                                 "domain",
                                                                                                                                 TestInterface.class,
                                                                                                                                 null,
                                                                                                                                 null,
                                                                                                                                 null));

        ProviderQos providerQos = new ProviderQos();
        List<CustomParameter> parameterList = Lists.newArrayList();
        parameterList.add(new CustomParameter("key1", "value1"));
        parameterList.add(new CustomParameter("key2", "value2"));
        providerQos.setCustomParameters(parameterList);

        EndpointAddressBase endpointAddress = new JoynrMessagingEndpointAddress(channelId);
        String participantId = "testParticipantId";
        String domain = "domain";
        capabilityEntry = new CapabilityEntry(domain, TestInterface.class, providerQos, endpointAddress, participantId);
        capabilityInformation = new CapabilityInformation(domain,
                                                          TestInterface.INTERFACE_NAME,
                                                          providerQos,
                                                          channelId,
                                                          participantId);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void addCapability() throws InterruptedException {

        RegistrationFuture future = localCapabilitiesDirectory.add(capabilityEntry);
        future.waitForFullRegistration(200);
        Mockito.verify(globalCapabilitiesClient).add(Mockito.any(Callback.class), Mockito.eq(capabilityInformation));
    }

    @Test
    public void addLocalOnlyCapability() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        capabilityInformation = new CapabilityInformation("test",
                                                          TestInterface.INTERFACE_NAME,
                                                          providerQos,
                                                          "chan",
                                                          "participantId");

        RegistrationFuture future = localCapabilitiesDirectory.add(capabilityEntry);
        future.waitForFullRegistration(10000);
        Mockito.verify(globalCapabilitiesClient, Mockito.never()).add(capabilityInformation);
    }

    private Answer<Future<List<CapabilityInformation>>> createAnswer(final List<CapabilityInformation> caps) {
        return new Answer<Future<List<CapabilityInformation>>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<List<CapabilityInformation>> answer(InvocationOnMock invocation) throws Throwable {
                Future<List<CapabilityInformation>> result = new Future<List<CapabilityInformation>>();
                Object[] args = invocation.getArguments();
                ((Callback<List<CapabilityInformation>>) args[0]).onSuccess(caps);
                result.onSuccess(caps);
                return result;
            }
        };
    }

    @SuppressWarnings("unchecked")
    @Test
    public void lookupWithScopeGlobalOnly() throws InterruptedException {
        List<CapabilityInformation> caps = new ArrayList<CapabilityInformation>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.GLOBAL_ONLY, 10000);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(Mockito.any(Callback.class),
                                                                                   Mockito.eq(domain1),
                                                                                   Mockito.eq(interfaceName1));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.any(String.class),
                                                                          Mockito.any(String.class));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        //add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        CapabilityEntry capEntry = new CapabilityEntry(domain1, interfaceName1, providerQos, "localParticipant");
        localCapabilitiesDirectory.add(capEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        //even deleting local cap entries shall have no effect, the global cap dir shall be invoked
        localCapabilitiesDirectory.remove(capEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        //add global entry
        CapabilityInformation capInfo = new CapabilityInformation(domain1,
                                                                  interfaceName1,
                                                                  new ProviderQos(),
                                                                  "channelId",
                                                                  "globalParticipant");
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(Mockito.any(Callback.class),
                                                                                   Mockito.eq(domain1),
                                                                                   Mockito.eq(interfaceName1));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(4)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        //now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(4)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        //and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAge(0);
        Thread.sleep(1);

        //now, another lookup call shall call the globalCapabilitiesClient, as the global cap dir is expired
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(5)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);
        Mockito.reset(globalCapabilitiesClient);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void lookupWithScopeLocalThenGlobal() throws InterruptedException {
        List<CapabilityInformation> caps = new ArrayList<CapabilityInformation>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_THEN_GLOBAL, 10000);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(Mockito.any(Callback.class),
                                                                                   Mockito.eq(domain1),
                                                                                   Mockito.eq(interfaceName1));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        //add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        CapabilityEntry capEntry = new CapabilityEntry(domain1, interfaceName1, providerQos, "localParticipant");
        localCapabilitiesDirectory.add(capEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        //add global entry
        CapabilityInformation capInfo = new CapabilityInformation(domain1,
                                                                  interfaceName1,
                                                                  new ProviderQos(),
                                                                  "channelId",
                                                                  "globalParticipant");
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(Mockito.any(Callback.class),
                                                                                   Mockito.eq(domain1),
                                                                                   Mockito.eq(interfaceName1));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));

        //now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir (as long as the cache is not expired)
        localCapabilitiesDirectory.remove(capEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));

        //now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));

        //and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAge(0);
        Thread.sleep(1);

        //now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.reset(globalCapabilitiesClient);
        Mockito.reset(capabilitiesCallback);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void lookupWithScopeLocalAndGlobal() throws InterruptedException {
        List<CapabilityInformation> caps = new ArrayList<CapabilityInformation>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_AND_GLOBAL, 500);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(Mockito.any(Callback.class),
                                                                                   Mockito.eq(domain1),
                                                                                   Mockito.eq(interfaceName1));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        //add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        CapabilityEntry capEntry = new CapabilityEntry(domain1, interfaceName1, providerQos, "localParticipant");
        localCapabilitiesDirectory.add(capEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        //add global entry
        CapabilityInformation capInfo = new CapabilityInformation(domain1,
                                                                  interfaceName1,
                                                                  new ProviderQos(),
                                                                  "channelId",
                                                                  "globalParticipant");
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(Mockito.any(Callback.class),
                                                                                   Mockito.eq(domain1),
                                                                                   Mockito.eq(interfaceName1));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));

        //now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));

        //and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAge(0);
        Thread.sleep(1);

        //now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(4)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1));
        Mockito.reset(globalCapabilitiesClient);
        Mockito.reset(capabilitiesCallback);
    }

    private class MyCollectionMatcher extends TypeSafeMatcher<Collection<CapabilityEntry>> {

        private int n;

        public MyCollectionMatcher(int n) {
            this.n = n;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("list has " + n + " entries");
        }

        @Override
        protected boolean matchesSafely(Collection<CapabilityEntry> item) {
            return item.size() == n;
        }

    }

    Matcher<Collection<CapabilityEntry>> hasNEntries(int n) {
        return new MyCollectionMatcher(n);
    }

    @Test
    public void removeCapabilities() throws InterruptedException {
        RegistrationFuture future = localCapabilitiesDirectory.add(capabilityEntry);
        future.waitForLocalRegistration(10000);
        localCapabilitiesDirectory.remove(capabilityEntry);
        Mockito.verify(globalCapabilitiesClient).remove(Mockito.eq(capabilityInformation.getParticipantId()));
    }

}
