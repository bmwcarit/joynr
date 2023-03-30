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
package io.joynr.messaging.sender;

import static joynr.system.RoutingTypes.RoutingTypesUtil.toAddressString;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.Address;

@Singleton
public class CcMessageSender extends AbstractMessageSender {

    @Inject(optional = true)
    @Named(MessagingPropertyKeys.GLOBAL_ADDRESS)
    private Address globalAddress = new Address();
    @Inject(optional = true)
    @Named(MessagingPropertyKeys.REPLY_TO_ADDRESS)
    private Address replyToAddress = new Address();

    @Inject
    public CcMessageSender(MessageRouter messageRouter) {
        super(messageRouter);
    }

    @Inject
    public void init() {
        setReplyToAddress(toAddressString(replyToAddress), toAddressString(globalAddress));
    }
}
