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
package io.joynr.messaging.inprocess;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;

public class InProcessLibjoynrMessagingSkeleton implements InProcessMessagingSkeleton {

    private static final Logger logger = LoggerFactory.getLogger(InProcessLibjoynrMessagingSkeleton.class);
    private final Dispatcher dispatcher;

    @Inject
    public InProcessLibjoynrMessagingSkeleton(Dispatcher dispatcher) {
        this.dispatcher = dispatcher;
    }

    @Override
    public void transmit(ImmutableMessage message, SuccessAction successAction, FailureAction failureAction) {
        logger.trace("InProcess call for message id {}", message.getId());

        try {
            dispatcher.messageArrived(message);
            successAction.execute();
        } catch (Exception e) {
            failureAction.execute(e);
        }
    }

    @Override
    public void init() {
        //do nothing
    }

    @Override
    public void shutdown() {
        //do nothing
    }
}
