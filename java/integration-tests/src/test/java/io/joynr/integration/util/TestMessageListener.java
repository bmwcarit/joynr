package io.joynr.integration.util;

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

import io.joynr.dispatching.WaitTillCondition;
import io.joynr.messaging.MessageArrivedListener;

import java.util.List;

import joynr.ImmutableMessage;

import com.google.common.collect.Lists;

public class TestMessageListener extends WaitTillCondition implements MessageArrivedListener {

    private List<Object> receivedPayloads = Lists.newArrayList();
    private List<Throwable> thrownErrors = Lists.newArrayList();

    public TestMessageListener(int numberOfMessagesExpected) {
        super(numberOfMessagesExpected);
    }

    public TestMessageListener(int numberOfMessagesExpected, int numberOfErrorsExpected) {
        super(numberOfMessagesExpected, numberOfErrorsExpected);
    }

    @Override
    public List<Object> getReceivedPayloads() {
        return receivedPayloads;
    }

    @Override
    public void messageArrived(ImmutableMessage message) {
        receivedPayloads.add(message);
        releaseSemaphorePermit();
    }

    public List<Throwable> getThrownErrors() {
        return thrownErrors;
    }

    @Override
    public void error(ImmutableMessage message, Throwable error) {
        releaseErrorSemaphorePermit();
        thrownErrors.add(error);
    }
}
