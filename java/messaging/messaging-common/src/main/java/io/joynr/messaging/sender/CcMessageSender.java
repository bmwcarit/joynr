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

import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;

@Singleton
public class CcMessageSender extends AbstractMessageSender {
    private String replyToAddress;
    private String globalAddress;

    @Inject
    public CcMessageSender(MessageRouter messageRouter,
                           ReplyToAddressProvider replyToAddressProvider,
                           GlobalAddressProvider globalAddressProvider) {
        super(messageRouter);
        replyToAddressProvider.registerGlobalAddressesReadyListener((address) -> addReplyToAddress(toAddressString(address.get())));
        globalAddressProvider.registerGlobalAddressesReadyListener((address) -> addGlobalAddress(toAddressString(address.get())));
    }

    private void addReplyToAddress(String replyToAddress) {
        synchronized (this) {
            this.replyToAddress = replyToAddress;
        }
        triggerSetReplyToAddressIfReady();
    }

    private void addGlobalAddress(String globalAddress) {
        synchronized (this) {
            this.globalAddress = globalAddress;
        }
        triggerSetReplyToAddressIfReady();
    }

    private void triggerSetReplyToAddressIfReady() {
        synchronized (this) {
            if (replyToAddress != null && globalAddress != null) {
                setReplyToAddress(replyToAddress, globalAddress);
            }
        }
    }
}
