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
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.concurrent.CompletableFuture;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.hivemq.client.mqtt.MqttClientState;
import com.hivemq.client.mqtt.MqttGlobalPublishFilter;
import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.exceptions.MqttClientStateException;
import com.hivemq.client.mqtt.mqtt3.Mqtt3AsyncClient;
import com.hivemq.client.mqtt.mqtt3.Mqtt3ClientConfig;
import com.hivemq.client.mqtt.mqtt3.Mqtt3RxClient;
import com.hivemq.client.mqtt.mqtt3.message.publish.Mqtt3Publish;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.reactivex.Flowable;

public class HivemqMqttClientTest {

    private static final long NOT_CONNECTED_RETRY_INTERVAL_MS = 60000;

    HivemqMqttClient client;
    @Mock
    private Mqtt3RxClient mockRxClient;
    @Mock
    private Mqtt3AsyncClient mockAsyncClient;
    private final int defaultKeepAliveTimerSec = 30;
    private final boolean defaultCleanSession = false;
    private final int defaultConnectionTimeoutSec = 60;
    private final int defaultReconnectDelayMs = 1000;
    @Mock
    private Mqtt3ClientConfig mockClientConfig;
    @Mock
    private Flowable<Mqtt3Publish> mockPublishesFlowable;
    @Mock
    private SuccessAction mockSuccessAction;
    @Mock
    private FailureAction mockFailureAction;
    private String testTopic;
    private byte[] testPayload;
    private CompletableFuture<Mqtt3Publish> publishFuture;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        testTopic = this.getClass().getName() + "-topic-" + System.currentTimeMillis();
        testPayload = (this.getClass().getName() + "-payload-" + System.currentTimeMillis()).getBytes();
        publishFuture = new CompletableFuture<Mqtt3Publish>();

        doReturn(mockClientConfig).when(mockRxClient).getConfig();
        doReturn(mockPublishesFlowable).when(mockRxClient).publishes(eq(MqttGlobalPublishFilter.ALL));
        doReturn(mockAsyncClient).when(mockRxClient).toAsync();
        doReturn(publishFuture).when(mockAsyncClient).publish(any(Mqtt3Publish.class));
        createDefaultClient();
    }

    private void createDefaultClient() {
        client = new HivemqMqttClient(mockRxClient,
                                      defaultKeepAliveTimerSec,
                                      defaultCleanSession,
                                      defaultConnectionTimeoutSec,
                                      defaultReconnectDelayMs,
                                      true,
                                      true);
    }

    @Test
    public void publishMessage_callsSuccessActionOnSuccess() {
        Mqtt3Publish expectedPublish = Mqtt3Publish.builder()
                                                   .topic(testTopic)
                                                   .qos(MqttQos.AT_LEAST_ONCE)
                                                   .payload(testPayload)
                                                   .build();
        doReturn(MqttClientState.CONNECTED).when(mockClientConfig).getState();

        client.publishMessage(testTopic,
                              testPayload,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              mockSuccessAction,
                              mockFailureAction);

        verify(mockSuccessAction, times(0)).execute();
        verify(mockAsyncClient, times(1)).publish(expectedPublish);

        publishFuture.complete(expectedPublish);

        verify(mockSuccessAction, times(1)).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
    }

    private void testPublishMessageDoesNotPublishAndCallsFailureAction() {
        JoynrDelayMessageException expectedException = new JoynrDelayMessageException(NOT_CONNECTED_RETRY_INTERVAL_MS,
                                                                                      "Publish failed: Mqtt client not connected.");

        client.publishMessage(testTopic,
                              testPayload,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              mockSuccessAction,
                              mockFailureAction);

        verify(mockFailureAction, times(1)).execute(eq(expectedException));
        verify(mockSuccessAction, times(0)).execute();
        verify(mockAsyncClient, times(0)).publish(any(Mqtt3Publish.class));
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
        Mqtt3Publish expectedPublish = Mqtt3Publish.builder()
                                                   .topic(testTopic)
                                                   .qos(MqttQos.AT_LEAST_ONCE)
                                                   .payload(testPayload)
                                                   .build();
        doReturn(MqttClientState.CONNECTED).when(mockClientConfig).getState();

        client.publishMessage(testTopic,
                              testPayload,
                              MqttQos.AT_LEAST_ONCE.getCode(),
                              mockSuccessAction,
                              mockFailureAction);

        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
        verify(mockAsyncClient, times(1)).publish(expectedPublish);

        publishFuture.completeExceptionally(publishException);

        verify(mockSuccessAction, times(0)).execute();
        verify(mockFailureAction, times(1)).execute(eq(expectedException));
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

}
