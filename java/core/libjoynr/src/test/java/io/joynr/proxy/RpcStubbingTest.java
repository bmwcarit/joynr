package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.Maps;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Provides;
import com.google.inject.Singleton;

import io.joynr.accesscontrol.AccessController;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.dispatcher.rpc.JoynrSyncInterface;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.dispatching.DispatcherTestModule;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.MessagingModule;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.websocket.WebSocketClientMessagingStubFactory;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.Promise;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.runtime.PropertyLoader;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
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
    @Mock
    private AccessController accessControllerMock;

    public static interface TestSync extends JoynrSyncInterface {
        public GpsLocation returnsGpsLocation();

        public List<GpsLocation> returnsGpsLocationList();

        public void takesTwoSimpleParams(@JoynrRpcParam("a") Integer a, @JoynrRpcParam("b") String b);

        public void noParamsNoReturnValue();
    }

    public static interface TestProvider extends JoynrInterface, JoynrProvider {
        public Promise<Deferred<GpsLocation>> returnsGpsLocation();

        public Promise<Deferred<List<GpsLocation>>> returnsGpsLocationList();

        public Promise<DeferredVoid> takesTwoSimpleParams(@JoynrRpcParam("a") Integer a, @JoynrRpcParam("b") String b);

        public Promise<DeferredVoid> noParamsNoReturnValue();
    }

    @Mock
    private ReplyCallerDirectory replyCallerDirectory;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private RequestReplyManager requestReplyManager;

    @Mock
    private TestProvider testMock;

    // private String domain;
    // private String interfaceName;
    private String fromParticipantId;
    private String toParticipantId;

    private Injector injector;

    private JoynrMessagingConnectorInvocationHandler connector;

    @Before
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "NP_NULL_PARAM_DEREF", justification = "NPE in test would fail test")
    public void setUp() throws JoynrCommunicationException, JoynrSendBufferFullException, JsonGenerationException,
                       JsonMappingException, IOException, JoynrMessageNotSentException {
        doReturn(TestProvider.class).when(testMock).getProvidedInterface();

        Deferred<GpsLocation> deferredGpsLocation = new Deferred<GpsLocation>();
        deferredGpsLocation.resolve(gpsValue);
        when(testMock.returnsGpsLocation()).thenReturn(new Promise<Deferred<GpsLocation>>(deferredGpsLocation));
        Deferred<List<GpsLocation>> deferredGpsLocationList = new Deferred<List<GpsLocation>>();
        deferredGpsLocationList.resolve(gpsList);
        when(testMock.returnsGpsLocationList()).thenReturn(new Promise<Deferred<List<GpsLocation>>>(deferredGpsLocationList));
        DeferredVoid deferredVoid = new DeferredVoid();
        deferredVoid.resolve();
        when(testMock.noParamsNoReturnValue()).thenReturn(new Promise<DeferredVoid>(deferredVoid));
        when(testMock.takesTwoSimpleParams(any(Integer.class), any(String.class))).thenReturn(new Promise<DeferredVoid>(deferredVoid));

        fromParticipantId = UUID.randomUUID().toString();
        toParticipantId = UUID.randomUUID().toString();

        // required to inject static members of JoynMessagingConnectorFactory
        injector = Guice.createInjector(new MessagingModule(),
                                        new JoynrPropertiesModule(PropertyLoader.loadProperties("defaultMessaging.properties")),
                                        new DispatcherTestModule(),
                                        new AbstractModule() {
                                            @Override
                                            protected void configure() {
                                                bind(AccessController.class).toInstance(accessControllerMock);
                                            }

                                            @Provides
                                            @Singleton
                                            Map<Class<? extends Address>, AbstractMessagingStubFactory> provideMessagingStubFactories(WebSocketClientMessagingStubFactory webSocketClientMessagingStubFactory,
                                                                                                                                      ChannelMessagingStubFactory channelMessagingStubFactory) {
                                                Map<Class<? extends Address>, AbstractMessagingStubFactory> factories = Maps.newHashMap();
                                                factories.put(WebSocketClientAddress.class,
                                                              webSocketClientMessagingStubFactory);
                                                factories.put(ChannelAddress.class, channelMessagingStubFactory);
                                                return factories;
                                            }
                                        });

        final RequestInterpreter requestInterpreter = injector.getInstance(RequestInterpreter.class);
        final RequestCallerFactory requestCallerFactory = injector.getInstance(RequestCallerFactory.class);

        when(requestReplyManager.sendSyncRequest(eq(fromParticipantId),
                                                 eq(toParticipantId),
                                                 any(Request.class),
                                                 any(SynchronizedReplyCaller.class),
                                                 eq(DEFAULT_TTL))).thenAnswer(new Answer<Reply>() {

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
                Callback<Reply> callback = new Callback<Reply>() {

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

        MessagingQos qosSettings = new MessagingQos(DEFAULT_TTL);
        JoynrMessagingConnectorFactory joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplyManager,
                                                                                                           replyCallerDirectory,
                                                                                                           subscriptionManager);
        connector = joynrMessagingConnectorFactory.create(fromParticipantId, toParticipantId, qosSettings);

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
                                                    eq(toParticipantId),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(DEFAULT_TTL));

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
                                                                                       String.class), args);

        // no return value expected
        assertNull(response);
        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                    eq(toParticipantId),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(DEFAULT_TTL));

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
                                                    eq(toParticipantId),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(DEFAULT_TTL));

        assertEquals(methodName, requestCaptor.getValue().getMethodName());
        assertEquals(0, requestCaptor.getValue().getParamDatatypes().length);
        verify(testMock).returnsGpsLocation();
        assertEquals(gpsValue, response);
    }

    @Test
    public void testWithListReturn() throws IOException, JoynrRuntimeException, ApplicationException,
                                    SecurityException, InstantiationException, IllegalAccessException,
                                    NoSuchMethodException {
        // Send
        String methodName = "returnsGpsLocationList";
        Object response = connector.executeSyncMethod(TestSync.class.getDeclaredMethod(methodName), new Object[]{});

        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                    eq(toParticipantId),
                                                    requestCaptor.capture(),
                                                    any(SynchronizedReplyCaller.class),
                                                    eq(DEFAULT_TTL));

        assertEquals(methodName, requestCaptor.getValue().getMethodName());
        assertEquals(0, requestCaptor.getValue().getParamDatatypes().length);
        verify(testMock).returnsGpsLocationList();
        assertEquals(gpsList, response);
    }

}
