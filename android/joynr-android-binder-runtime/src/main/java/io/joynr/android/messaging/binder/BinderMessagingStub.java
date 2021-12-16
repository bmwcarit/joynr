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
package io.joynr.android.messaging.binder;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.TimeUnit;

import io.joynr.android.binder.BinderService;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;

public class BinderMessagingStub implements IMessagingStub {
    private static final Logger logger = LoggerFactory.getLogger(BinderMessagingStub.class);

    private static final String BINDER_SERVICE_CLASSNAME = BinderService.class.getName();

    private Address toAddress;
    private Context context;

    public BinderMessagingStub(Context context, Address toAddress) {
        this.toAddress = toAddress;
        this.context = context;
    }

    @Override
    public void transmit(ImmutableMessage message, SuccessAction successAction, FailureAction failureAction) {
        logger.debug(">>> OUTGOING >>> {}", message);

        if (!message.isTtlAbsolute()) {
            throw new JoynrRuntimeException("Relative TTL not supported");
        }

        long timeout = message.getTtlMs() - System.currentTimeMillis();
        byte[] serializedMessage = message.getSerializedMessage();

        connectAndTransmitData(serializedMessage,
                               (BinderAddress) toAddress,
                               timeout,
                               TimeUnit.MILLISECONDS,
                               successAction,
                               failureAction);
    }

    protected void connectAndTransmitData(byte[] data,
                                        BinderAddress toClientAddress,
                                        long timeout,
                                        TimeUnit unit,
                                        SuccessAction successAction,
                                        FailureAction failureAction) {
        Intent intent = new Intent();
        intent.setComponent(new ComponentName(toClientAddress.getPackageName(), BINDER_SERVICE_CLASSNAME));
        ServiceConnection connection = new BinderServiceConnection(context, data, successAction, failureAction);

        // bind to service appropriately
        new BinderServiceStub(context, intent, connection, toClientAddress).createBinderService();
    }
}
