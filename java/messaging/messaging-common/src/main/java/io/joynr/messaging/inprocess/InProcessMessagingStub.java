/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import com.google.inject.Inject;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;

public class InProcessMessagingStub implements IMessagingStub {

    private final InProcessMessagingSkeleton skeleton;

    @Inject
    public InProcessMessagingStub(InProcessMessagingSkeleton skeleton) {
        this.skeleton = skeleton;
    }

    @Override
    public void transmit(ImmutableMessage message, SuccessAction successAction, FailureAction failureAction) {
        skeleton.transmit(message, successAction, failureAction);
    }
}
