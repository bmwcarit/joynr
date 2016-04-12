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
import static org.mockito.Matchers.eq;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.JoynrRuntime;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.name.Names;

@Ignore
@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryTest {
    private static final String TEST_URL = "http://testUrl";
    @Mock
    JoynrRuntime runtime;
    @Mock
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;
    @Mock
    private MessageRouter messageRouter;
    @Mock
    private Dispatcher dispatcher;
    @Mock
    private ProxyBuilderFactory proxyBuilderFactoryMock;
    @Mock
    protected DiscoveryEntryStore localDiscoveryEntryStoreMock;
    @Mock
    protected DiscoveryEntryStore globalDiscoveryEntryCacheMock;

    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private ChannelAddress channelAddress;
    private String channelAddressSerialized;
    private DiscoveryEntry discoveryEntry;
    private GlobalDiscoveryEntry globalDiscoveryEntry;

    public interface TestInterface extends JoynrInterface {
        public static final String INTERFACE_NAME = "interfaceName";
    }

    @SuppressWarnings("unchecked")
    @Before
    public void setUp() throws Exception {

        channelAddress = new ChannelAddress(TEST_URL, "testChannelId");
        ObjectMapper objectMapper = new ObjectMapper();
        channelAddressSerialized = objectMapper.writeValueAsString(channelAddress);

        Answer<Void> answer = new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                return null;
            }

        };
        Mockito.doAnswer(answer).when(globalCapabilitiesClient).add(Mockito.any(Callback.class),
                                                                    Mockito.any(GlobalDiscoveryEntry.class));

        String discoveryDirectoriesDomain = "io.joynr";
        String capabilitiesDirectoryParticipantId = "capDir_participantId";
        String capabiltitiesDirectoryChannelId = "dirchannelId";
        String domainAccessControllerParticipantId = "domainAccessControllerParticipantId";
        String domainAccessControllerChannelId = "domainAccessControllerChannelId";

        Injector injector = Guice.createInjector(new JsonMessageSerializerModule(), new AbstractModule() {
            @Override
            protected void configure() {
                requestStaticInjection(CapabilityUtils.class);
                bind(Address.class).annotatedWith(Names.named(ClusterControllerRuntimeModule.GLOBAL_ADDRESS))
                                   .toInstance(channelAddress);
            }
        });

        localCapabilitiesDirectory = new LocalCapabilitiesDirectoryImpl(discoveryDirectoriesDomain,
                                                                        capabilitiesDirectoryParticipantId,
                                                                        new ChannelAddress(TEST_URL,
                                                                                           capabiltitiesDirectoryChannelId),
                                                                        domainAccessControllerParticipantId,
                                                                        new ChannelAddress(TEST_URL,
                                                                                           domainAccessControllerChannelId),
                                                                        injector.getProvider(Key.get(Address.class,
                                                                                                     Names.named(ClusterControllerRuntimeModule.GLOBAL_ADDRESS))),
                                                                        localDiscoveryEntryStoreMock,
                                                                        globalDiscoveryEntryCacheMock,
                                                                        messageRouter,
                                                                        proxyBuilderFactoryMock,
                                                                        new ObjectMapper());

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] parameterList = { new CustomParameter("key1", "value1"),
                new CustomParameter("key2", "value2") };
        providerQos.setCustomParameters(parameterList);

        String participantId = "testParticipantId";
        String domain = "domain";
        discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                            domain,
                                            TestInterface.INTERFACE_NAME,
                                            participantId,
                                            providerQos,
                                            System.currentTimeMillis());
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        channelAddressSerialized);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void addCapability() throws InterruptedException {

        localCapabilitiesDirectory.add(discoveryEntry);

        Mockito.verify(globalCapabilitiesClient, Mockito.timeout(200)).add(Mockito.any(Callback.class),
                                                                           Mockito.eq(globalDiscoveryEntry));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void addLocalOnlyCapability() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        "test",
                                                        TestInterface.INTERFACE_NAME,
                                                        "participantId",
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        channelAddressSerialized);

        localCapabilitiesDirectory.add(discoveryEntry);
        Mockito.verify(globalCapabilitiesClient, Mockito.timeout(10000).never()).add(any(Callback.class),
                                                                                     any(GlobalDiscoveryEntry.class));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void addGlobalCapSucceeds_NextAddShallNotAddGlobalAgain() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        String participantId = LocalCapabilitiesDirectoryTest.class.getName()
                + ".addGlobalCapSucceeds_NextAddShallNotAddGlobalAgain";
        String domain = "testDomain";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 TestInterface.INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis());
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        channelAddressSerialized);

        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry);
        promise.then(new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                Mockito.doAnswer(createAddAnswerWithSuccess())
                       .when(globalCapabilitiesClient)
                       .add(Mockito.any(Callback.class), Mockito.eq(globalDiscoveryEntry));

                Mockito.verify(globalDiscoveryEntryCacheMock).add(Mockito.eq(globalDiscoveryEntry));
                Mockito.verify(globalCapabilitiesClient).add(Mockito.any(Callback.class),
                                                             Mockito.eq(globalDiscoveryEntry));
                Mockito.reset(globalCapabilitiesClient);
                localCapabilitiesDirectory.add(discoveryEntry);
                Mockito.verify(globalCapabilitiesClient, Mockito.timeout(200).never())
                       .add(Mockito.any(Callback.class), Mockito.eq(globalDiscoveryEntry));
            }

            @Override
            public void onRejection(JoynrException error) {
                Assert.fail("adding capability failed: " + error);
            }
        });

    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void addGlobalCapFails_NextAddShallAddGlobalAgain() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        String participantId = LocalCapabilitiesDirectoryTest.class.getName() + ".addLocalAndThanGlobalShallWork";
        String domain = "testDomain";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 TestInterface.INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis());
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        channelAddressSerialized);

        Mockito.doAnswer(createAddAnswerWithError())
               .when(globalCapabilitiesClient)
               .add(Mockito.any(Callback.class), Mockito.eq(globalDiscoveryEntry));

        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry);
        promise.then(new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                Mockito.verify(globalDiscoveryEntryCacheMock, Mockito.never()).add(Mockito.eq(globalDiscoveryEntry));
                Mockito.verify(globalCapabilitiesClient).add(Mockito.any(Callback.class),
                                                             Mockito.eq(globalDiscoveryEntry));
                Mockito.reset(globalCapabilitiesClient);
                localCapabilitiesDirectory.add(discoveryEntry);
                Mockito.verify(globalCapabilitiesClient, Mockito.timeout(200)).add(Mockito.any(Callback.class),
                                                                                   Mockito.eq(globalDiscoveryEntry));

            }

            @Override
            public void onRejection(JoynrException error) {

            }
        });

    }

    private Answer<Future<List<GlobalDiscoveryEntry>>> createAnswer(final List<GlobalDiscoveryEntry> caps) {
        return new Answer<Future<List<GlobalDiscoveryEntry>>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<List<GlobalDiscoveryEntry>> answer(InvocationOnMock invocation) throws Throwable {
                Future<List<GlobalDiscoveryEntry>> result = new Future<List<GlobalDiscoveryEntry>>();
                Object[] args = invocation.getArguments();
                ((Callback<List<GlobalDiscoveryEntry>>) args[0]).onSuccess(caps);
                result.onSuccess(caps);
                return result;
            }
        };
    }

    private Answer<Future<Void>> createAddAnswerWithSuccess() {
        return new Answer<Future<Void>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                result.onSuccess(null);
                return result;
            }
        };
    }

    private Answer<Future<Void>> createAddAnswerWithError() {
        return new Answer<Future<Void>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onFailure(new JoynrRuntimeException("Simulating a JoynrRuntimeException on callback"));
                result.onSuccess(null);
                return result;
            }
        };
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void lookupWithScopeGlobalOnly() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.GLOBAL_ONLY, 1000);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(Mockito.any(Callback.class),
                       Mockito.eq(domain1),
                       Mockito.eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.any(String.class),
                                                                          Mockito.any(String.class),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis());
        localCapabilitiesDirectory.add(discoveryEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        // even deleting local cap entries shall have no effect, the global cap dir shall be invoked
        localCapabilitiesDirectory.remove(discoveryEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                channelAddressSerialized);
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(Mockito.any(Callback.class),
                       Mockito.eq(domain1),
                       Mockito.eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(4)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(4)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAgeMs(0);
        Thread.sleep(1);

        // now, another lookup call shall call the globalCapabilitiesClient, as the global cap dir is expired
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(5)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));
        Mockito.reset(capabilitiesCallback);
        Mockito.reset(globalCapabilitiesClient);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void lookupWithScopeLocalThenGlobal() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_THEN_GLOBAL, 10000);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(Mockito.any(Callback.class),
                       Mockito.eq(domain1),
                       Mockito.eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis());
        localCapabilitiesDirectory.add(discoveryEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                channelAddressSerialized);
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(Mockito.any(Callback.class),
                       Mockito.eq(domain1),
                       Mockito.eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.remove(discoveryEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAgeMs(0);
        Thread.sleep(1);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.reset(globalCapabilitiesClient);
        Mockito.reset(capabilitiesCallback);
    }

    @Test(timeout = 1000)
    public void lookupByParticipantIdWithScopeLocalSync() throws InterruptedException {
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        String participantId1 = "participantId1";
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_ONLY, 10000);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        DiscoveryEntry expectedDiscoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                   domain1,
                                                                   interfaceName1,
                                                                   participantId1,
                                                                   providerQos,
                                                                   System.currentTimeMillis());
        localCapabilitiesDirectory.add(expectedDiscoveryEntry);
        DiscoveryEntry retrievedCapabilityEntry = localCapabilitiesDirectory.lookup(participantId1, discoveryQos);
        Assert.assertEquals(expectedDiscoveryEntry, retrievedCapabilityEntry);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void lookupWithScopeLocalAndGlobal() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_AND_GLOBAL, 500);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(Mockito.any(Callback.class),
                       Mockito.eq(domain1),
                       Mockito.eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(1)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(0))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis());
        localCapabilitiesDirectory.add(discoveryEntry);
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(2)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(0)));
        Mockito.verify(capabilitiesCallback, Mockito.times(1))
               .processCapabilitiesReceived(Mockito.argThat(hasNEntries(1)));

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                channelAddressSerialized);
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(Mockito.any(Callback.class),
                       Mockito.eq(domain1),
                       Mockito.eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(3)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAgeMs(0);
        Thread.sleep(1);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(domain1, interfaceName1, discoveryQos, capabilitiesCallback);
        Mockito.verify(globalCapabilitiesClient, Mockito.times(4)).lookup(Mockito.any(Callback.class),
                                                                          Mockito.eq(domain1),
                                                                          Mockito.eq(interfaceName1),
                                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        Mockito.reset(globalCapabilitiesClient);
        Mockito.reset(capabilitiesCallback);
    }

    private class MyCollectionMatcher extends TypeSafeMatcher<Collection<DiscoveryEntry>> {

        private int n;

        public MyCollectionMatcher(int n) {
            this.n = n;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("list has " + n + " entries");
        }

        @Override
        protected boolean matchesSafely(Collection<DiscoveryEntry> item) {
            return item.size() == n;
        }

    }

    Matcher<Collection<DiscoveryEntry>> hasNEntries(int n) {
        return new MyCollectionMatcher(n);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void removeCapabilities() throws InterruptedException {
        localCapabilitiesDirectory.add(discoveryEntry);
        localCapabilitiesDirectory.remove(discoveryEntry);
        Mockito.verify(globalCapabilitiesClient, Mockito.timeout(1000))
               .remove(Mockito.any(Callback.class), Mockito.eq(globalDiscoveryEntry.getParticipantId()));
    }

}
