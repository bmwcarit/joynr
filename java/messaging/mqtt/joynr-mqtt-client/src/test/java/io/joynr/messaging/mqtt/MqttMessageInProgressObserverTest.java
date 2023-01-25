/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessageInProgressObserverTest {

    @Mock
    private MqttMessagingSkeleton mqttMessagingSkeletonMock;

    private MqttMessageInProgressObserver createTestSubject(boolean backpressureEnabled,
                                                            int maxIncomingMqttRequests,
                                                            int reEnableMessageAcknowledgementTreshold,
                                                            int receiveMaximum) {
        return new MqttMessageInProgressObserver(backpressureEnabled,
                                                 maxIncomingMqttRequests,
                                                 reEnableMessageAcknowledgementTreshold,
                                                 receiveMaximum);
    }

    @Test
    public void testAcknowledgementAlwaysAllowedWhenBackpressureDisabled() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(false, maxIncomingMqttRequests, 20, receiveMaximum);
        for (int i = 1; i <= (maxIncomingMqttRequests * 2); i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
    }

    @Test
    public void testMessagingskeletonNotNotifiedWhenBackpressureDisabled() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementTreshold = 20;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(false,
                                                                  maxIncomingMqttRequests,
                                                                  reEnableMessageAcknowledgementTreshold,
                                                                  receiveMaximum);
        for (int i = 1; i <= maxIncomingMqttRequests; i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
        for (int i = 1; i <= maxIncomingMqttRequests; i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        verify(mqttMessagingSkeletonMock, never()).acknowledgeOutstandingPublishes();
    }

    @Test
    public void testBackpressureDisabledWhenReceiveMaximumGreaterThanMaximumIncomingRequests() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 101;
        MqttMessageInProgressObserver subject = createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum);
        for (int i = 1; i <= maxIncomingMqttRequests; i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenMaximumIncomingRequestsZero() {
        int maxIncomingMqttRequests = 0;
        int receiveMaximum = 101;
        createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenLowerBackpressureThresholdTooHigh() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementTreshold = 81;
        int receiveMaximum = 20;
        createTestSubject(true, maxIncomingMqttRequests, reEnableMessageAcknowledgementTreshold, receiveMaximum);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenLowerBackpressureThresholdBelowZero() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementTreshold = -1;
        int receiveMaximum = 20;
        createTestSubject(true, maxIncomingMqttRequests, reEnableMessageAcknowledgementTreshold, receiveMaximum);
    }

    @Test
    public void testAcknowledgementDisallowedWhenUpperThresholdReached() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum);
        for (int i = 1; i <= (maxIncomingMqttRequests - receiveMaximum); i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
        assertFalse(subject.canMessageBeAcknowledged("latestMessage"));
    }

    @Test
    public void testAcknowledgementReEnabledWhenLowerThresholdReached() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementTreshold = 20;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum);
        subject.registerMessagingSkeleton(mqttMessagingSkeletonMock);
        for (int i = 1; i <= (maxIncomingMqttRequests - receiveMaximum); i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
        String latestMessageId = "latestMessage";
        String secondLatestMessageId = "secondLatestMessage";
        assertFalse(subject.canMessageBeAcknowledged(latestMessageId));
        // Acknowledgement should not be triggered until lower threshold is reached
        for (int i = 1; i <= 20; i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        assertFalse(subject.canMessageBeAcknowledged(secondLatestMessageId));
        for (int i = 1; i <= (maxIncomingMqttRequests - receiveMaximum - reEnableMessageAcknowledgementTreshold); i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        verify(mqttMessagingSkeletonMock, never()).acknowledgeOutstandingPublishes();
        // Messages should not be able to be acknowledged a second time
        for (int i = 1; i <= (maxIncomingMqttRequests - receiveMaximum - reEnableMessageAcknowledgementTreshold); i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        subject.decrementMessagesInProgress(secondLatestMessageId);
        verify(mqttMessagingSkeletonMock, never()).acknowledgeOutstandingPublishes();
        subject.decrementMessagesInProgress(latestMessageId);
        verify(mqttMessagingSkeletonMock, times(1)).acknowledgeOutstandingPublishes();
    }

}
