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
package io.joynr.proxy;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.name.Names;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.context.JoynrMessageScopeModule;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerFactory;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingStubFactory;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.Promise;
import io.joynr.provider.ProviderCallback;
import io.joynr.runtime.PropertyLoader;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;

/**
 * Simulates consumer-side call to JoynMessagingConnectorInvocationHandler, with the request being
 *
 * @author david.katz
 *
 */
@RunWith(MockitoJUnitRunner.class)
public class RpcStubbingTest {
    private static final GpsLocation gpsValue = new GpsLocation(1.0d,
                                                                2.0d,
                                                                0d,
                                                                GpsFixEnum.MODE2D,
                                                                0d,
                                                                0d,
                                                                0d,
                                                                0d,
                                                                0l,
                                                                0l,
                                                                0);

    private static final List<GpsLocation> gpsList = Arrays.asList(new GpsLocation[]{ gpsValue,
            new GpsLocation(3.0d, 4.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0) });

    private static final long DEFAULT_TTL = 2000L;

    public static interface TestSync {
        public GpsLocation returnsGpsLocation();

        public List<GpsLocation> returnsGpsLocationList();

        public void takesTwoSimpleParams(Integer a, String b);

        public void noParamsNoReturnValue();
    }

    @JoynrInterface(provider = TestProvider.class, provides = TestProvider.class, name = TestProvider.INTERFACE_NAME)
    public static interface TestProvider extends JoynrProvider {
        public static final String INTERFACE_NAME = "rpcstubbing/test";

        public Promise<Deferred<GpsLocation>> returnsGpsLocation();

        public Promise<Deferred<List<GpsLocation>>> returnsGpsLocationList();

        public Promise<DeferredVoid> takesTwoSimpleParams(Integer a, String b);

        public Promise<DeferredVoid> noParamsNoReturnValue();
    }

    @Mock
    private ReplyCallerDirectory replyCallerDirectory;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private RequestReplyManager requestReplyManager;
    @Mock
    private StatelessAsyncIdCalculator statelessAsyncIdCalculator;
    @Mock
    private TestProvider testMock;

    // private String domain;
    // private String interfaceName;
    private String fromParticipantId;
    private String toParticipantId;
    private DiscoveryEntryWithMetaInfo toDiscoveryEntry;
    private final MessagingQos messagingQos = new MessagingQos(DEFAULT_TTL);

    private Injector injector;

    private JoynrMessagingConnectorInvocationHandler connector;

    @Before
    public void setUp() throws JoynrCommunicationException, JoynrSendBufferFullException, JsonGenerationException,
                        JsonMappingException, IOException, JoynrMessageNotSentException {
        Deferred<GpsLocation> deferredGpsLocation = new Deferred<GpsLocation>();
        deferredGpsLocation.resolve(gpsValue);
        when(testMock.returnsGpsLocation()).thenReturn(new Promise<Deferred<GpsLocation>>(deferredGpsLocation));
        Deferred<List<GpsLocation>> deferredGpsLocationList = new Deferred<List<GpsLocation>>();
        deferredGpsLocationList.resolve(gpsList);
        when(testMock.returnsGpsLocationList()).thenReturn(new Promise<Deferred<List<GpsLocation>>>(deferredGpsLocationList));
        DeferredVoid deferredVoid = new DeferredVoid();
        deferredVoid.resolve();
        when(testMock.noParamsNoReturnValue()).thenReturn(new Promise<DeferredVoid>(deferredVoid));
        when(testMock.takesTwoSimpleParams(any(Integer.class),
                                           any(String.class))).thenReturn(new Promise<DeferredVoid>(deferredVoid));

        fromParticipantId = createUuidString();
        toParticipantId = createUuidString();
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        toDiscoveryEntry.setParticipantId(toParticipantId);

        // required to inject static members of JoynMessagingConnectorFactory
        injector = Guice.createInjector(new JoynrPropertiesModule(PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE)),
                                        new JsonMessageSerializerModule(),
                                        new AbstractModule() {

                                            @Override
                                            protected void configure() {
                                                requestStaticInjection(RpcUtils.class);
                                                install(new JoynrMessageScopeModule());
                                                MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory;
                                                messagingStubFactory = MapBinder.newMapBinder(binder(),
                                                                                              new TypeLiteral<Class<? extends Address>>() {
                                                                                              },
                                                                                              new TypeLiteral<AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>>() {
                                                                                              },
                                                                                              Names.named(MessagingStubFactory.MIDDLEWARE_MESSAGING_STUB_FACTORIES));
                                                messagingStubFactory.addBinding(InProcessAddress.class)
                                                                    .to(InProcessMessagingStubFactory.class);
                                            }
                                        });

        final RequestInterpreter requestInterpreter = injector.getInstance(RequestInterpreter.class);
        final RequestCallerFactory requestCallerFactory = injector.getInstance(RequestCallerFactory.class);

        when(requestReplyManager.sendSyncRequest(eq(fromParticipantId),
                                                 eq(toDiscoveryEntry),
                                                 any(Request.class),
                                                 any(SynchronizedReplyCaller.class),
                                                 eq(messagingQos))).thenAnswer(new Answer<Reply>() {

                                                     @Override
                                                     public Reply answer(InvocationOnMock invocation) throws Throwable {
                                                         RequestCaller requestCaller = requestCallerFactory.create(testMock);
                                                         Object[] args = invocation.getArguments();
                                                         Request request = null;
                                                         for (Object arg : args) {
                                                             if (arg instanceof Request) {
                                                                 request = (Request) arg;
                                                                 break;
                                                             }
                                                         }
                                                         final Future<Reply> future = new Future<Reply>();
                                                         ProviderCallback<Reply> callback = new ProviderCallback<Reply>() {

                                                             @Override
                                                             public void onSuccess(Reply result) {
                                                                 future.onSuccess(result);
                                                             }

                                                             @Override
                                                             public void onFailure(JoynrException error) {
                                                                 future.onFailure(error);
                                                             }
                                                         };
                                                         requestInterpreter.execute(callback, requestCaller, request);
                                                         return future.get();
                                                     }
                                                 });

        JoynrMessagingConnectorFactory joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplyManager,
                                                                                                           replyCallerDirectory,
                                                                                                           subscriptionManager,
                                                                                                           statelessAsyncIdCalculator);
        connector = joynrMessagingConnectorFactory.create(fromParticipantId,
                                                          new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                          messagingQos,
                                                          null);

    }

    @Test
    public void testWithoutArguments() throws IOException, JoynrRuntimeException, SecurityException,
                                       InstantiationException, IllegalAccessException, NoSuchMethodException,
                                       ApplicationException {
        // Send
        String methodName = "noParamsNoReturnValue";
        Object result = connector.executeSyncMethod(TestSync.class.getDeclaredMethod(methodName, new Class<?>[]{}),
                                                    new Object[]{});

        assertNull(result);

        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                    eq(toDiscoveryEntry),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(messagingQos));

        verify(testMock).noParamsNoReturnValue();
        assertEquals(methodName, requestCaptor.getValue().getMethodName());
        assertEquals(0, requestCaptor.getValue().getParamDatatypes().length);
    }

    @Test
    public void testWithArguments() throws IOException, JoynrRuntimeException, ApplicationException, SecurityException,
                                    InstantiationException, IllegalAccessException, NoSuchMethodException {
        // Send

        String methodName = "takesTwoSimpleParams";
        Object[] args = new Object[]{ 3, "abc" };
        Object response = connector.executeSyncMethod(TestSync.class.getDeclaredMethod(methodName,
                                                                                       Integer.class,
                                                                                       String.class),
                                                      args);

        // no return value expected
        assertNull(response);
        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                    eq(toDiscoveryEntry),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(messagingQos));

        assertEquals(methodName, requestCaptor.getValue().getMethodName());
        assertEquals(2, requestCaptor.getValue().getParamDatatypes().length);
        assertArrayEquals(args, requestCaptor.getValue().getParams());
        verify(testMock).takesTwoSimpleParams(3, "abc");

    }

    @Test
    public void testWithReturn() throws IOException, JoynrRuntimeException, ApplicationException, SecurityException,
                                 InstantiationException, IllegalAccessException, NoSuchMethodException {
        // Send
        String methodName = "returnsGpsLocation";
        Object response = connector.executeSyncMethod(TestSync.class.getDeclaredMethod(methodName), new Object[]{});

        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                    eq(toDiscoveryEntry),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(messagingQos));

        assertEquals(methodName, requestCaptor.getValue().getMethodName());
        assertEquals(0, requestCaptor.getValue().getParamDatatypes().length);
        verify(testMock).returnsGpsLocation();
        assertEquals(gpsValue, response);
    }

    @Test
    public void testWithListReturn() throws IOException, JoynrRuntimeException, ApplicationException, SecurityException,
                                     InstantiationException, IllegalAccessException, NoSuchMethodException {
        // Send
        String methodName = "returnsGpsLocationList";
        Object response = connector.executeSyncMethod(TestSync.class.getDeclaredMethod(methodName), new Object[]{});

        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                    eq(toDiscoveryEntry),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(messagingQos));

        assertEquals(methodName, requestCaptor.getValue().getMethodName());
        assertEquals(0, requestCaptor.getValue().getParamDatatypes().length);
        verify(testMock).returnsGpsLocationList();
        assertEquals(gpsList, response);
    }

}
