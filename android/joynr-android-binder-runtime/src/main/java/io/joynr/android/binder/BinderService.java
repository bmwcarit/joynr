/*-
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
package io.joynr.android.binder;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import com.google.inject.Key;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Set;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.android.messaging.binder.JoynrBinder;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.routing.MessageRouter;
import joynr.ImmutableMessage;
import joynr.Message;

public class BinderService extends Service {
    private static final Logger logger = LoggerFactory.getLogger(BinderService.class);

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }


    private final JoynrBinder.Stub binder = new JoynrBinder.Stub() {
        @Override
        public void transmit(byte[] serializedMessage) {

            try {
                ImmutableMessage message = new ImmutableMessage(serializedMessage);

                logger.debug("<<< INCOMING <<<< {}", message);

                Set<JoynrMessageProcessor> messageProcessors = AndroidBinderRuntime.getInjector().getInstance(new Key<Set<JoynrMessageProcessor>>() {
                });
                if (messageProcessors != null) {
                    for (JoynrMessageProcessor processor : messageProcessors) {
                        message = processor.processIncoming(message);
                    }
                }

                if (Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
                    message.setReceivedFromGlobal(true);
                }

                MessageRouter messageRouter = AndroidBinderRuntime.getInjector().getInstance(MessageRouter.class);
                messageRouter.route(message);

            } catch (Exception error) {
                logger.error("BinderService message not processed: {}", error.getMessage());
            }
        }
    };


}
