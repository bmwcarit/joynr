/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.android.messaging.binder;

import io.joynr.messaging.AbstractMessagingSkeletonFactory;
import io.joynr.messaging.IMessagingSkeleton;

public class BinderMessagingSkeletonFactory extends AbstractMessagingSkeletonFactory {

    /**
     * Because {@link io.joynr.messaging.routing.AbstractMessageRouter} performSubscriptionOperation() method crashes when asking
     * for binder messaging skeleton, we instantiate a dummy skeleton that does nothing.
     */
    private class BinderMessagingSkeleton implements IMessagingSkeleton {

        @Override
        public void init() {
        }

        @Override
        public void shutdown() {

        }
    }

    public BinderMessagingSkeletonFactory() {
        super();
        messagingSkeletonList.add(new BinderMessagingSkeleton());
    }

    @Override
    public void shutdown() {
    }
}
