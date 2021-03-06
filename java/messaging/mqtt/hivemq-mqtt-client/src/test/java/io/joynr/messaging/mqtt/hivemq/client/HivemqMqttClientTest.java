/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt.hivemq.client;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.concurrent.CompletableFuture;
import java.util.Optional;
import java.util.OptionalLong;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import com.hivemq.client.internal.mqtt.message.publish.MqttPublish;
import com.hivemq.client.internal.mqtt.message.publish.MqttPublishResult.MqttQos1Result;
import com.hivemq.client.mqtt.datatypes.MqttTopic;
import com.hivemq.client.mqtt.MqttClientState;
import com.hivemq.client.mqtt.MqttGlobalPublishFilter;
import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.exceptions.MqttClientStateException;
import com.hivemq.client.mqtt.mqtt5.Mqtt5AsyncClient;
import com.hivemq.client.mqtt.mqtt5.Mqtt5ClientConfig;
import com.hivemq.client.mqtt.mqtt5.Mqtt5RxClient;
import com.hivemq.client.mqtt.mqtt5.message.connect.Mqtt5Connect;
import com.hivemq.client.mqtt.mqtt5.message.connect.connack.Mqtt5ConnAck;
import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;
import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5PublishResult;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.statusmetrics.ConnectionStatusMetricsImpl;
import io.reactivex.Flowable;
import io.reactivex.Single;
import io.reactivex.functions.Consumer;

public class HivemqMqttClientTest {

    private static final long NOT_CONNECTED_RETRY_INTERVAL_MS = 5000;

    HivemqMqttClient client;
    @Mock
    private Mqtt5RxClient mockRxClient;
    @Mock
    private Mqtt5AsyncClient mockAsyncClient;
    private final int defaultKeepAliveTimerSec = 30;
    private final boolean defaultCleanSession = false;
    private final int defaultConnectionTimeoutSec = 60;
    private final int defaultReconnectDelayMs = 1000;
    private final String defaultGbid = "HivemqMqttClientTest-GBID";
    @Mock
    private Mqtt5ClientConfig mockClientConfig;
    @Mock
    private Flowable<Mqtt5Publish> mockPublishesFlowable;
    @Captor
    ArgumentCaptor<Consumer<? super Mqtt5Publish>> publishesFlowableOnNextCaptor;
    @Captor
    ArgumentCaptor<Consumer<? super Throwable>> publishesFlowableOnErrorCaptor;
    @Mock
    private IMqttMessagingSkeleton mockSkeleton;
    @Mock
    private SuccessAction mockSuccessAction;
    @Mock
    private FailureAction mockFailureAction;
    @Mock
    private Single<Mqtt5ConnAck> mockConnectSingle;
    private String testTopic;
    private byte[] testPayload;
    private long testExpiryIntervalSec;
    private CompletableFuture<Mqtt5PublishResult> publishFuture;

    @Mock
    private ConnectionStatusMetricsImpl mockConnectionStatusMetrics;

    @Rule
    public ExpectedException thrown = ExpectedException.none();

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        testTopic = this.getClass().getName() + "-topic-" + System.currentTimeMillis();
        testPayload = (this.getClass().getName() + "-payload-" + System.currentTimeMillis()).getBytes();
        testExpiryIntervalSec = 60;
        publishFuture = new CompletableFuture<Mqtt5PublishResult>();

        doReturn(mockClientConfig).when(mockRxClient).getConfig();
        doReturn(mockPublishesFlowable).when(mockRxClient).publishes(eq(MqttGlobalPublishFilter.ALL));

        doReturn(mockAsyncClient).when(mockRxClient).toAsync();
        doReturn(publishFuture).when(mockAsyncClient).publish(any(Mqtt5Publish.class));
        createDefaultClient();
    }

    private void createDefaultClient() {
        client = new HivemqMqttClient(mockRxClient,
                                      defaultKeepAliveTimerSec,
                                      defaultCleanSession,
                                      defaultConnectionTimeoutSec,
                                      defaultReconnectDelayMs,
                                      true,
                                      true,
                                      defaultGbid,
                                      mockConnectionStatusMetrics);
    }

    @Test
    public void publishMessage_callsSuccessActionOnSuccess() {
        Mqtt5Publish expectedPublish = Mqtt5Publish.builder()
                                                   .topic(testTopic)
                                                   .qos(MqttQos.AT_LEAST_ONCE)
                                                   .payload(testPayload)
                                                   .messageExpiryInterval(testExpiryIntervalSec)
                                                   .build();
        MqttQos1Result mockResult = new MqttQos1Result((MqttPublish) expectedPublish, null, null);
        doReturn(MqttClientState.CONNECTED).when(mockClientConfig).getState();

        client.publishMessage(testTopic,
                              testPayload,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              testExpiryIntervalSec,
                              mockSuccessAction,
                              mockFailureAction);

        verify(mockConnectionStatusMetrics, times(0)).increaseSentMessages();
        verify(mockSuccessAction, times(0)).execute();
        verify(mockAsyncClient, times(1)).publish(expectedPublish);

        publishFuture.complete(mockResult);

        verify(mockSuccessAction, times(1)).execute();
        verify(mockConnectionStatusMetrics, times(1)).increaseSentMessages();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
    }

    private void testPublishMessageDoesNotPublishAndCallsFailureAction() {
        JoynrDelayMessageException expectedException = new JoynrDelayMessageException(NOT_CONNECTED_RETRY_INTERVAL_MS,
                                                                                      "Publish failed: Mqtt client not connected.");

        client.publishMessage(testTopic,
                              testPayload,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              testExpiryIntervalSec,
                              mockSuccessAction,
                              mockFailureAction);

        verify(mockFailureAction, times(1)).execute(eq(expectedException));
        verify(mockSuccessAction, times(0)).execute();
        verify(mockAsyncClient, times(0)).publish(any(Mqtt5Publish.class));
        verify(mockConnectionStatusMetrics, times(0)).increaseSentMessages();
    }

    @Test
    public void publishMessage_doesNotPublishAndCallsFailureActionIfNotConnected_DISCONNECTED() {
        doReturn(MqttClientState.DISCONNECTED).when(mockClientConfig).getState();
        testPublishMessageDoesNotPublishAndCallsFailureAction();
    }

    @Test
    public void publishMessage_doesNotPublishAndCallsFailureActionIfNotConnected_CONNECTING() {
        doReturn(MqttClientState.CONNECTING).when(mockClientConfig).getState();
        testPublishMessageDoesNotPublishAndCallsFailureAction();
    }

    @Test
    public void publishMessage_doesNotPublishAndCallsFailureActionIfNotConnected_CONNECTING_RECONNECT() {
        doReturn(MqttClientState.CONNECTING_RECONNECT).when(mockClientConfig).getState();
        testPublishMessageDoesNotPublishAndCallsFailureAction();
    }

    @Test
    public void publishMessage_doesNotPublishAndCallsFailureActionIfNotConnected_DISCONNECTED_RECONNECT() {
        doReturn(MqttClientState.DISCONNECTED_RECONNECT).when(mockClientConfig).getState();
        testPublishMessageDoesNotPublishAndCallsFailureAction();
    }

    private void testPublishMessageWithException(Exception publishException, Exception expectedException) {
        Mqtt5Publish expectedPublish = Mqtt5Publish.builder()
                                                   .topic(testTopic)
                                                   .qos(MqttQos.AT_LEAST_ONCE)
                                                   .payload(testPayload)
                                                   .messageExpiryInterval(testExpiryIntervalSec)
                                                   .build();
        doReturn(MqttClientState.CONNECTED).when(mockClientConfig).getState();

        client.publishMessage(testTopic,
                              testPayload,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              testExpiryIntervalSec,
                              mockSuccessAction,
                              mockFailureAction);

        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
        verify(mockAsyncClient, times(1)).publish(expectedPublish);

        publishFuture.completeExceptionally(publishException);

        verify(mockSuccessAction, times(0)).execute();
        verify(mockFailureAction, times(1)).execute(eq(expectedException));
        verify(mockConnectionStatusMetrics, times(0)).increaseSentMessages();
    }

    @Test
    public void publishMessage_callsFailureActionOnMqttClientStateException() {
        MqttClientStateException publishException = new MqttClientStateException("test exception");
        JoynrDelayMessageException expectedException = new JoynrDelayMessageException(NOT_CONNECTED_RETRY_INTERVAL_MS,
                                                                                      "Publish failed: "
                                                                                              + publishException.toString());

        testPublishMessageWithException(publishException, expectedException);
    }

    @Test
    public void publishMessage_callsFailureActionOnOtherError() {
        Exception publishException = new Exception("test exception");
        JoynrDelayMessageException expectedException = new JoynrDelayMessageException("Publish failed: "
                + publishException.toString());

        testPublishMessageWithException(publishException, expectedException);
    }

    @Test
    public void publishMessage_callsFailureActionOnErrorResult() {
        Exception publishException = new Exception("test exception");
        JoynrDelayMessageException expectedException = new JoynrDelayMessageException("Publish failed: "
                + publishException.toString());

        Mqtt5Publish expectedPublish = Mqtt5Publish.builder()
                                                   .topic(testTopic)
                                                   .qos(MqttQos.AT_LEAST_ONCE)
                                                   .payload(testPayload)
                                                   .messageExpiryInterval(testExpiryIntervalSec)
                                                   .build();
        MqttQos1Result mockResult = new MqttQos1Result((MqttPublish) expectedPublish, publishException, null);
        doReturn(MqttClientState.CONNECTED).when(mockClientConfig).getState();

        client.publishMessage(testTopic,
                              testPayload,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              testExpiryIntervalSec,
                              mockSuccessAction,
                              mockFailureAction);

        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
        verify(mockAsyncClient, times(1)).publish(expectedPublish);

        publishFuture.complete(mockResult);

        verify(mockSuccessAction, times(0)).execute();
        verify(mockFailureAction, times(1)).execute(eq(expectedException));
        verify(mockConnectionStatusMetrics, times(0)).increaseSentMessages();
    }

    @Test
    public void incomingMessageIncreasesReceivedMessagesCount() throws Exception {
        // Capturing the real publishes Consumers did not work because verify() with ArgumentCaptor calls
        // the real implementation with null values which is not allowed:
        // java.lang.NullPointerException: onNext is null
        //     at io.reactivex.internal.functions.ObjectHelper.requireNonNull(ObjectHelper.java)
        //     at io.reactivex.Flowable.subscribe(Flowable.java)
        //     at io.reactivex.Flowable.subscribe(Flowable.java)
        //     at io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientTest.incomingMessageIncreasesReceivedMessagesCount
        // verify(mockPublishesFlowable).subscribe(publishesFlowableOnNextCaptor.capture(), publishesFlowableOnErrorCaptor.capture());
        // publishesFlowableOnNextCaptor.getValue().accept(mockPublish);

        client.setMessageListener(mockSkeleton);
        verify(mockConnectionStatusMetrics, times(0)).increaseReceivedMessages();

        Mqtt5Publish mockPublish = mock(Mqtt5Publish.class);
        doReturn(new byte[0]).when(mockPublish).getPayloadAsBytes();
        Optional<ByteBuffer> optionalByteBuffer = Optional.ofNullable(null);
        doReturn(optionalByteBuffer).when(mockPublish).getPayload();
        doReturn(MqttTopic.of("topic")).when(mockPublish).getTopic();
        doReturn(false).when(mockPublish).isRetain();
        doReturn(null).when(mockPublish).getQos();
        doReturn(OptionalLong.of(0l)).when(mockPublish).getMessageExpiryInterval();
        Method handleIncomingMessage = client.getClass().getDeclaredMethod("handleIncomingMessage", Mqtt5Publish.class);
        handleIncomingMessage.setAccessible(true);
        handleIncomingMessage.invoke(client, mockPublish);

        verify(mockConnectionStatusMetrics, times(1)).increaseReceivedMessages();
    }

    @Test
    public void startIncreasesNumberOfConnectionAttempts() {
        doAnswer(new Answer<MqttClientState>() {
            int callCount = 0;

            @Override
            public MqttClientState answer(InvocationOnMock invocation) throws Throwable {
                if (callCount < 2) {
                    callCount++;
                    return MqttClientState.DISCONNECTED;
                }
                return MqttClientState.CONNECTED;
            }
        }).when(mockClientConfig).getState();
        client.setMessageListener(mockSkeleton);
        verify(mockConnectionStatusMetrics, times(0)).increaseConnectionAttempts();

        // Mocking the connect behavior did not work because doReturn() calls the real implementation of timeout():
        // java.lang.NullPointerException: unit is null
        //     at io.reactivex.internal.functions.ObjectHelper.requireNonNull(ObjectHelper.java)
        //     at io.reactivex.Single.timeout0(Single.java)
        //     at io.reactivex.Single.timeout(Single.java)
        //     at io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientTest.startIncreasesNumberOfConnectionAttempts
        // doReturn(mockConnectSingle).when(mockRxClient).connect(any(Mqtt5Connect.class));
        // doReturn(mockConnectSingle).when(mockConnectSingle).timeout(anyLong(), any(TimeUnit.class));
        // doReturn(mockConnectSingle).when(mockConnectSingle).doOnSuccess(Matchers.<Consumer<? super Mqtt5ConnAck>>any());

        doThrow(new RuntimeException()).when(mockRxClient).connect(any(Mqtt5Connect.class));
        client.start();
        verify(mockConnectionStatusMetrics, times(1)).increaseConnectionAttempts();
    }

    @Test
    public void publishMessage_throwsWhenMaxMessageSizeExceeded() throws Exception {
        final int maxMessageSize = 100;
        client = new HivemqMqttClient(mockRxClient,
                                      defaultKeepAliveTimerSec,
                                      defaultCleanSession,
                                      defaultConnectionTimeoutSec,
                                      defaultReconnectDelayMs,
                                      true,
                                      true,
                                      defaultGbid,
                                      mockConnectionStatusMetrics);
        doReturn(MqttClientState.CONNECTED).when(mockClientConfig).getState();

        Field maxMsgSizeBytesField = HivemqMqttClient.class.getDeclaredField("maxMsgSizeBytes");
        maxMsgSizeBytesField.setAccessible(true);
        maxMsgSizeBytesField.set(client, 100);

        byte[] largeSerializedMessage = new byte[maxMessageSize + 1];
        thrown.expect(JoynrMessageNotSentException.class);
        thrown.expectMessage("Publish failed: maximum allowed message size of " + maxMessageSize
                + " bytes exceeded, actual size is " + largeSerializedMessage.length + " bytes");

        client.publishMessage(testTopic,
                              largeSerializedMessage,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              testExpiryIntervalSec,
                              mockSuccessAction,
                              mockFailureAction);
        verify(mockSuccessAction, times(0)).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
    }

    @Test
    public void publishMessage_doesNotThrowWhenMaxMessageSizeNotExceeded() throws Exception {
        final int maxMessageSize = 100;
        client = new HivemqMqttClient(mockRxClient,
                                      defaultKeepAliveTimerSec,
                                      defaultCleanSession,
                                      defaultConnectionTimeoutSec,
                                      defaultReconnectDelayMs,
                                      true,
                                      true,
                                      defaultGbid,
                                      mockConnectionStatusMetrics);

        Field maxMsgSizeBytesField = HivemqMqttClient.class.getDeclaredField("maxMsgSizeBytes");
        maxMsgSizeBytesField.setAccessible(true);
        maxMsgSizeBytesField.set(client, 100);

        byte[] shortSerializedMessage = new byte[maxMessageSize];
        Mqtt5Publish expectedPublish = Mqtt5Publish.builder()
                                                   .topic(testTopic)
                                                   .qos(MqttQos.AT_LEAST_ONCE)
                                                   .payload(shortSerializedMessage)
                                                   .messageExpiryInterval(testExpiryIntervalSec)
                                                   .build();
        MqttQos1Result mockResult = new MqttQos1Result((MqttPublish) expectedPublish, null, null);
        doReturn(MqttClientState.CONNECTED).when(mockClientConfig).getState();

        client.publishMessage(testTopic,
                              shortSerializedMessage,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              testExpiryIntervalSec,
                              mockSuccessAction,
                              mockFailureAction);
        publishFuture.complete(mockResult);

        verify(mockSuccessAction, times(1)).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
    }

}
