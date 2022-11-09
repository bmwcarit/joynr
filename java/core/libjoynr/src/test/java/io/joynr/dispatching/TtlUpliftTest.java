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
package io.joynr.dispatching;

import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Method;
import java.util.Optional;
import java.util.Properties;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.common.ExpiryDate;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.dispatching.subscription.AttributePollInterpreter;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.PublicationManagerImpl;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import io.joynr.provider.ProviderContainer;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.runtime.JoynrInjectionConstants;
import joynr.BroadcastSubscriptionRequest;
import joynr.ImmutableMessage;
import joynr.MulticastPublication;
import joynr.MutableMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.tests.testProvider;

@RunWith(MockitoJUnitRunner.class)
public class TtlUpliftTest {
    private static final long TTL = 1000;
    private static final long TTL_UPLIFT_MS = 10000;
    private static final long NO_TTL_UPLIFT = 0;
    private static final long SUBSCRIPTION_UPLIFT_MS = 300;
    private static final long LARGE_EXPIRY_DATE_MS = 9007199254740991L;

    private static final String PROVIDER_PARTICIPANT_ID = "providerParticipantId";
    private static final String PROXY_PARTICIPANT_ID = "proxyParticipantId";
    private static final String SUBSCRIPTION_ID = "PublicationTest_id";

    private String fromParticipantId;
    private String toParticipantId;
    private Request request;
    private String payload;
    private ExpiryDate expiryDate;
    private MessagingQos messagingQos;

    private MutableMessageFactory messageFactory;
    private MutableMessageFactory messageFactoryWithTtlUplift;

    private ScheduledExecutorService cleanupScheduler;
    private ScheduledExecutorService cleanupSchedulerSpy;
    private RequestCaller requestCaller;
    private PublicationManagerImpl publicationManager;
    private PublicationManagerImpl publicationManagerWithTtlUplift;

    @Mock
    private AttributePollInterpreter attributePollInterpreter;
    @Mock
    private ProviderDirectory providerDirectory;
    @Mock
    private DispatcherImpl dispatcher;
    @Mock
    private testProvider provider;
    @Mock
    private AbstractSubscriptionPublisher subscriptionPublisher;
    @Mock
    private ProviderContainer providerContainer;

    @Captor
    ArgumentCaptor<MessagingQos> messagingQosCaptor;
    @Captor
    ArgumentCaptor<Long> longCaptor;

    private String valueToPublish = "valuePublished";

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException {
        fromParticipantId = "sender";
        toParticipantId = "receiver";
        cleanupScheduler = new ScheduledThreadPoolExecutor(1);
        cleanupSchedulerSpy = Mockito.spy(cleanupScheduler);

        Module defaultModule = Modules.override(new JoynrPropertiesModule(new Properties()))
                                      .with(new JsonMessageSerializerModule(), new AbstractModule() {

                                          @Override
                                          protected void configure() {
                                              bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS))
                                                              .toInstance(NO_TTL_UPLIFT);
                                              requestStaticInjection(Request.class);
                                              Multibinder<JoynrMessageProcessor> joynrMessageProcessorMultibinder = Multibinder.newSetBinder(binder(),
                                                                                                                                             new TypeLiteral<JoynrMessageProcessor>() {
                                                                                                                                             });
                                              joynrMessageProcessorMultibinder.addBinding()
                                                                              .toInstance(new JoynrMessageProcessor() {
                                                                                  @Override
                                                                                  public MutableMessage processOutgoing(MutableMessage joynrMessage) {
                                                                                      return joynrMessage;
                                                                                  }

                                                                                  @Override
                                                                                  public ImmutableMessage processIncoming(ImmutableMessage joynrMessage) {
                                                                                      return joynrMessage;
                                                                                  }
                                                                              });
                                              bind(PublicationManager.class).to(PublicationManagerImpl.class);
                                              bind(AttributePollInterpreter.class).toInstance(attributePollInterpreter);
                                              bind(Dispatcher.class).toInstance(dispatcher);
                                              bind(ProviderDirectory.class).toInstance(providerDirectory);
                                              bind(RoutingTable.class).toInstance(Mockito.mock(RoutingTable.class));
                                              bind(ScheduledExecutorService.class).annotatedWith(Names.named(JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP))
                                                                                  .toInstance(cleanupSchedulerSpy);
                                          }

                                      });
        Injector injector = Guice.createInjector(defaultModule);

        messageFactory = injector.getInstance(MutableMessageFactory.class);

        Module ttlUpliftModule = Modules.override(defaultModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS))
                                .toInstance(TTL_UPLIFT_MS);
            }
        });
        Injector injectorWithTtlUplift = Guice.createInjector(ttlUpliftModule);
        messageFactoryWithTtlUplift = injectorWithTtlUplift.getInstance(MutableMessageFactory.class);

        requestCaller = new RequestCallerFactory().create(provider);
        when(providerContainer.getProviderProxy()).thenReturn(requestCaller.getProxy());
        when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);
        Deferred<String> valueToPublishDeferred = new Deferred<String>();
        valueToPublishDeferred.resolve(valueToPublish);
        Promise<Deferred<String>> valueToPublishPromise = new Promise<Deferred<String>>(valueToPublishDeferred);
        doReturn(Optional.of(valueToPublishPromise)).when(attributePollInterpreter)
                                                    .execute(any(ProviderContainer.class), any(Method.class));

        Module subcriptionUpliftModule = Modules.override(defaultModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS))
                                .toInstance(SUBSCRIPTION_UPLIFT_MS);
            }
        });
        Injector injectorWithPublicationUplift = Guice.createInjector(subcriptionUpliftModule);
        publicationManager = (PublicationManagerImpl) injector.getInstance(PublicationManager.class);
        publicationManagerWithTtlUplift = (PublicationManagerImpl) injectorWithPublicationUplift.getInstance(PublicationManager.class);

        payload = "payload";
        Method method = TestProvider.class.getMethod("methodWithStrings", new Class[]{ String.class });
        request = new Request(method.getName(), new String[]{ payload }, method.getParameterTypes());
        messagingQos = new MessagingQos(TTL);
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
    }

    @Test
    public void testDefaultTtlUpliftMs() {
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactory.createRequest(fromParticipantId,
                                                              toParticipantId,
                                                              request,
                                                              messagingQos);

        long expiryDateValue = expiryDate.getValue();
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_Request() {
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createRequest(fromParticipantId,
                                                                           toParticipantId,
                                                                           request,
                                                                           messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_Reply_noUplift() {
        Reply reply = new Reply();
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createReply(fromParticipantId,
                                                                         toParticipantId,
                                                                         reply,
                                                                         messagingQos);

        long expiryDateValue = expiryDate.getValue();
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_OneWayRequest() {
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createOneWayRequest(fromParticipantId,
                                                                                 toParticipantId,
                                                                                 request,
                                                                                 messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_SubscriptionReply() {
        SubscriptionReply subscriptionReply = new SubscriptionReply();
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createSubscriptionReply(fromParticipantId,
                                                                                     toParticipantId,
                                                                                     subscriptionReply,
                                                                                     messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_SubscriptionRequest() {
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest();
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createSubscriptionRequest(fromParticipantId,
                                                                                       toParticipantId,
                                                                                       subscriptionRequest,
                                                                                       messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_Publication() {
        SubscriptionPublication subscriptionPublication = new SubscriptionPublication();
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createPublication(fromParticipantId,
                                                                               toParticipantId,
                                                                               subscriptionPublication,
                                                                               messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_SubscriptionStop() {
        SubscriptionStop subscriptionStop = new SubscriptionStop();
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createSubscriptionStop(fromParticipantId,
                                                                                    toParticipantId,
                                                                                    subscriptionStop,
                                                                                    messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs_Multicast() {
        MulticastPublication multicastPublication = new MulticastPublication();
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        MutableMessage message = messageFactoryWithTtlUplift.createMulticast(fromParticipantId,
                                                                             multicastPublication,
                                                                             messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMsWithLargeTtl() {
        MessagingQos messagingQos = new MessagingQos(LARGE_EXPIRY_DATE_MS);
        MutableMessage message = messageFactoryWithTtlUplift.createRequest(fromParticipantId,
                                                                           toParticipantId,
                                                                           request,
                                                                           messagingQos);
        long expiryDateValue = LARGE_EXPIRY_DATE_MS;
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);

        messagingQos = new MessagingQos(LARGE_EXPIRY_DATE_MS - TTL_UPLIFT_MS);
        message = messageFactoryWithTtlUplift.createRequest(fromParticipantId, toParticipantId, request, messagingQos);
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);

        messagingQos = new MessagingQos(LARGE_EXPIRY_DATE_MS - TTL_UPLIFT_MS - System.currentTimeMillis());
        message = messageFactoryWithTtlUplift.createRequest(fromParticipantId, toParticipantId, request, messagingQos);
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);

        messagingQos = new MessagingQos(LARGE_EXPIRY_DATE_MS - TTL_UPLIFT_MS - System.currentTimeMillis() + 1);
        message = messageFactoryWithTtlUplift.createRequest(fromParticipantId, toParticipantId, request, messagingQos);
        MutableMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    private static class MessagingQosMatcher implements ArgumentMatcher<MessagingQos> {

        private long expectedPublicationTtlMs;
        private String describeTo;

        private MessagingQosMatcher(long expectedPublicationTtlMs) {
            this.expectedPublicationTtlMs = expectedPublicationTtlMs;
            describeTo = "";
        }

        @Override
        public boolean matches(MessagingQos argument) {
            if (argument == null) {
                describeTo = "argument was null";
                return false;
            }
            if (!argument.getClass().equals(MessagingQos.class)) {
                describeTo = "unexpected class: " + argument.getClass();
                return false;
            }
            MessagingQos actual = (MessagingQos) argument;
            if (actual.getRoundTripTtl_ms() == expectedPublicationTtlMs) {
                return true;
            }
            describeTo = "expected roundTripTtlMs: " + expectedPublicationTtlMs + ", actual: "
                    + actual.getRoundTripTtl_ms();
            return false;
        }

        @Override
        public String toString() {
            return describeTo;
        }
    }

    private void verifySubscriptionReplyTtl(long expectedSubscriptionReplyTtl, long toleranceMs) {
        verify(dispatcher, times(1)).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                           eq(PROXY_PARTICIPANT_ID),
                                                           any(SubscriptionReply.class),
                                                           messagingQosCaptor.capture());
        MessagingQos capturedMessagingQos = messagingQosCaptor.getValue();
        long diff = Math.abs(expectedSubscriptionReplyTtl - capturedMessagingQos.getRoundTripTtl_ms());
        assertTrue("TTL of subscriptionReply=" + capturedMessagingQos.getRoundTripTtl_ms() + " differs " + diff
                + "ms (more than " + toleranceMs + "ms) from the expected value=" + expectedSubscriptionReplyTtl,
                   (diff <= toleranceMs));
    }

    private void verifyCleanupSchedulerDelay(long expectedDelay, long toleranceMs) {
        verify(cleanupSchedulerSpy, times(1)).schedule(any(Runnable.class), longCaptor.capture(), any(TimeUnit.class));
        long capturedLong = longCaptor.getValue();
        long diff = expectedDelay - capturedLong;
        assertTrue("Delay for cleanupScheduler=" + capturedLong + " differs " + diff + "ms (more than " + toleranceMs
                + "ms) from the expected value=" + expectedDelay, (diff <= toleranceMs));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testAttributeSubscriptionWithoutTtlUplift() throws Exception {
        long validityMs = 300;
        long publicationTtlMs = 1000;
        long toleranceMs = 50;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setValidityMs(validityMs);
        qos.setPublicationTtlMs(publicationTtlMs);

        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);

        verifySubscriptionReplyTtl(validityMs, toleranceMs);

        verifyCleanupSchedulerDelay(validityMs, toleranceMs);

        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        // sending initial value plus the attributeValueChanged
        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 argThat(new MessagingQosMatcher(publicationTtlMs)));

        Thread.sleep(validityMs + toleranceMs);
        reset(dispatcher);

        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        verify(dispatcher,
               timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    private void testAttributeSubscriptionWithTtlUplift(OnChangeSubscriptionQos qos,
                                                        long sleepDurationMs,
                                                        long expectedSubscriptionReplyTtl,
                                                        long expectedPublicationTtlMs) throws InterruptedException {
        final long toleranceMs = 60;
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManagerWithTtlUplift.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                               PROVIDER_PARTICIPANT_ID,
                                                               subscriptionRequest);

        verifySubscriptionReplyTtl(expectedSubscriptionReplyTtl, toleranceMs);
        if (qos.getExpiryDateMs() != SubscriptionQos.NO_EXPIRY_DATE) {
            verifyCleanupSchedulerDelay(expectedSubscriptionReplyTtl, toleranceMs);
        } else {
            verify(cleanupSchedulerSpy, times(0)).schedule(any(Runnable.class), anyLong(), any(TimeUnit.class));
        }

        publicationManagerWithTtlUplift.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        Thread.sleep(sleepDurationMs + toleranceMs);

        publicationManagerWithTtlUplift.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);
        // sending initial value plus 2 times the attributeValueChanged
        verify(dispatcher,
               times(3)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                     argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                     any(SubscriptionPublication.class),
                                                     argThat(new MessagingQosMatcher(expectedPublicationTtlMs)));

        Thread.sleep(SUBSCRIPTION_UPLIFT_MS);
        reset(dispatcher);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testAttributeSubscriptionWithTtlUplift() throws Exception {
        long validityMs = 300;
        long publicationTtlMs = 1000;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setValidityMs(validityMs);
        qos.setPublicationTtlMs(publicationTtlMs);

        long expectedSubscriptionReplyTtl = validityMs;
        long expectedPublicationTtlMs = publicationTtlMs;

        testAttributeSubscriptionWithTtlUplift(qos, validityMs, expectedSubscriptionReplyTtl, expectedPublicationTtlMs);

        publicationManagerWithTtlUplift.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        verify(dispatcher,
               timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  any(MessagingQos.class));
    }

    @Ignore
    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testAttributeSubscriptionWithTtlUpliftWithNoExpiryDate() throws Exception {
        long validityMs = 300;
        long publicationTtlMs = 1000;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE);
        qos.setPublicationTtlMs(publicationTtlMs);

        long expectedSubscriptionReplyTtl = LARGE_EXPIRY_DATE_MS;
        long expectedPublicationTtlMs = publicationTtlMs;

        testAttributeSubscriptionWithTtlUplift(qos, validityMs, expectedSubscriptionReplyTtl, expectedPublicationTtlMs);

        publicationManagerWithTtlUplift.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        verify(dispatcher,
               timeout(300).times(1)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  argThat(new MessagingQosMatcher(expectedPublicationTtlMs)));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testAttributeSubscriptionWithTtlUpliftWithLargeExpiryDate() throws Exception {
        long validityMs = 300;
        long publicationTtlMs = 1000;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setExpiryDateMs(LARGE_EXPIRY_DATE_MS - SUBSCRIPTION_UPLIFT_MS + 1);
        qos.setPublicationTtlMs(publicationTtlMs);

        long expectedSubscriptionReplyTtl = qos.getExpiryDateMs() - System.currentTimeMillis();
        long expectedPublicationTtlMs = publicationTtlMs;

        testAttributeSubscriptionWithTtlUplift(qos, validityMs, expectedSubscriptionReplyTtl, expectedPublicationTtlMs);

        publicationManagerWithTtlUplift.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        verify(dispatcher,
               timeout(300).times(1)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  argThat(new MessagingQosMatcher(expectedPublicationTtlMs)));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testBroadcastSubscriptionWithoutTtlUplift() throws Exception {
        long validityMs = 300;
        long toleranceMs = 50;
        long publicationTtlMs = 1000;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setValidityMs(validityMs);
        qos.setPublicationTtlMs(publicationTtlMs);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "location",
                                                                                   null,
                                                                                   qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);

        verifySubscriptionReplyTtl(validityMs, toleranceMs);
        verifyCleanupSchedulerDelay(validityMs, toleranceMs);

        publicationManager.broadcastOccurred(SUBSCRIPTION_ID, null, valueToPublish);

        // sending the broadcastOccurred
        verify(dispatcher, times(1)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 argThat(new MessagingQosMatcher(publicationTtlMs)));

        Thread.sleep(validityMs + toleranceMs);
        reset(dispatcher);

        publicationManager.broadcastOccurred(SUBSCRIPTION_ID, null, valueToPublish);

        verify(dispatcher,
               timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    private void testBroadcastSubscriptionWithTtlUplift(OnChangeSubscriptionQos qos,
                                                        long sleepDurationMs,
                                                        long expectedSubscriptionReplyTtl,
                                                        long expectedPublicationTtlMs) throws InterruptedException {
        final long toleranceMs = 50;
        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "location",
                                                                                   null,
                                                                                   qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManagerWithTtlUplift.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                               PROVIDER_PARTICIPANT_ID,
                                                               subscriptionRequest);

        verifySubscriptionReplyTtl(expectedSubscriptionReplyTtl, toleranceMs);
        if (qos.getExpiryDateMs() != SubscriptionQos.NO_EXPIRY_DATE) {
            verifyCleanupSchedulerDelay(expectedSubscriptionReplyTtl, toleranceMs);
        } else {
            verify(cleanupSchedulerSpy, times(0)).schedule(any(Runnable.class), anyLong(), any(TimeUnit.class));
        }

        publicationManagerWithTtlUplift.broadcastOccurred(SUBSCRIPTION_ID, null, valueToPublish);

        Thread.sleep(sleepDurationMs + toleranceMs);

        publicationManagerWithTtlUplift.broadcastOccurred(SUBSCRIPTION_ID, null, valueToPublish);
        // sending 2 times the broadcastOccurred
        verify(dispatcher,
               times(2)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                     argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                     any(SubscriptionPublication.class),
                                                     argThat(new MessagingQosMatcher(expectedPublicationTtlMs)));

        Thread.sleep(SUBSCRIPTION_UPLIFT_MS);
        reset(dispatcher);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testBroadcastSubscriptionWithTtlUplift() throws Exception {
        long validityMs = 300;
        long publicationTtlMs = 1000;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setValidityMs(validityMs);
        qos.setPublicationTtlMs(publicationTtlMs);

        long expectedSubscriptionReplyTtl = validityMs;
        long expectedPublicationTtlMs = publicationTtlMs;

        testBroadcastSubscriptionWithTtlUplift(qos, validityMs, expectedSubscriptionReplyTtl, expectedPublicationTtlMs);

        publicationManagerWithTtlUplift.broadcastOccurred(SUBSCRIPTION_ID, null, valueToPublish);

        verify(dispatcher,
               timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  any(MessagingQos.class));
    }

    @Ignore
    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testBroadcastSubscriptionWithTtlUpliftWithNoExpiryDate() throws Exception {
        long validityMs = 300;
        long publicationTtlMs = 1000;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE);
        qos.setPublicationTtlMs(publicationTtlMs);

        long expectedSubscriptionReplyTtl = LARGE_EXPIRY_DATE_MS;
        long expectedPublicationTtlMs = publicationTtlMs;

        testBroadcastSubscriptionWithTtlUplift(qos, validityMs, expectedSubscriptionReplyTtl, expectedPublicationTtlMs);

        publicationManagerWithTtlUplift.broadcastOccurred(SUBSCRIPTION_ID, null, valueToPublish);

        verify(dispatcher,
               timeout(300).times(1)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  argThat(new MessagingQosMatcher(expectedPublicationTtlMs)));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void testBroadcastSubscriptionWithTtlUpliftWithLargeExpiryDate() throws Exception {
        long validityMs = 300;
        long publicationTtlMs = 1000;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setExpiryDateMs(LARGE_EXPIRY_DATE_MS - SUBSCRIPTION_UPLIFT_MS + 1);
        qos.setPublicationTtlMs(publicationTtlMs);

        long expectedSubscriptionReplyTtl = qos.getExpiryDateMs() - System.currentTimeMillis();
        long expectedPublicationTtlMs = publicationTtlMs;

        testBroadcastSubscriptionWithTtlUplift(qos, validityMs, expectedSubscriptionReplyTtl, expectedPublicationTtlMs);

        publicationManagerWithTtlUplift.broadcastOccurred(SUBSCRIPTION_ID, null, valueToPublish);

        verify(dispatcher,
               timeout(300).times(1)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  argThat(mySet -> mySet.contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  argThat(new MessagingQosMatcher(expectedPublicationTtlMs)));
    }
}
