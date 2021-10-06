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
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;

import com.google.inject.Injector;
import com.google.inject.Key;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.android.messaging.binder.JoynrBinder;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.routing.MessageRouter;
import joynr.ImmutableMessage;

/**
 * Android service used by {@link AndroidBinderRuntime} to communicate between instances using joynr
 * on Android.
 */
public class BinderService extends Service {
    private static final Logger logger = LoggerFactory.getLogger(BinderService.class);
    private static final long DELAY_MILLISECOND_FOR_RETRY_CHECK_CC = 1000L;
    private static final int MESSAGE_WHAT = 0;

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    private List<ImmutableMessage> messagesWaitingProcessing = new ArrayList();

    private final JoynrBinder.Stub binder = new JoynrBinder.Stub() {
        @Override
        public void transmit(byte[] serializedMessage) {

            try {
                ImmutableMessage message = new ImmutableMessage(serializedMessage);

                logger.debug("BinderService <<< INCOMING <<<< {}", message);
                Injector injector = AndroidBinderRuntime.getInjector();
                if (injector != null) {
                    processMessage(injector, message);
                } else {
                    messagesWaitingProcessing.add(message);
                    removeAndSendMessageDelayedToHandler();
                }

            } catch (Exception error) {
                logger.error("BinderService transmit message not processed: {}", error.getMessage());
            }
        }
    };

    /**
     * Process's a message passed as parameter sending it to the message processors
     * available in the guice injector.
     * @param injector the guice injector being used
     * @param message the message that you want to process
     */
    private void processMessage(Injector injector, ImmutableMessage message) {

        Set<JoynrMessageProcessor> messageProcessors = injector.getInstance(new Key<Set<JoynrMessageProcessor>>() {
        });
        if (messageProcessors != null) {
            for (JoynrMessageProcessor processor : messageProcessors) {
                message = processor.processIncoming(message);
            }
        }

        message.setReceivedFromGlobal(true);

        MessageRouter messageRouter = AndroidBinderRuntime.getInjector().getInstance(MessageRouter.class);

        messageRouter.routeIn(message);
    }

    /**
     * Handler responsible for checking if the guice injector used by the joynr runtime
     * gets available so that messages received before the runtime gets initialized are processed.
     */
    private Handler checkInjectorRuntimeAvailabilityHandler = new Handler(Looper.getMainLooper()) {

        @Override
        public void handleMessage(android.os.Message msg) {

            try {
                Injector injector = AndroidBinderRuntime.getInjector();
                if (injector != null) {
                    messagesWaitingProcessing.forEach(immutableMessage -> {
                        processMessage(injector, immutableMessage);
                    });
                    messagesWaitingProcessing.clear();
                    logger.debug("BinderService Runtime available. Processing {} messages", messagesWaitingProcessing.size());
                } else {
                    logger.debug("BinderService Runtime not available. Checking again in 1 second...");
                    removeAndSendMessageDelayedToHandler();
                }
            } catch (Exception error) {
                logger.error("BinderService handleMessage message not processed: {}", error.getMessage());
            }
        }
    };

    /**
     * Helper function that removes possible messages waiting to be processed in the handler
     * and send a new fresh message delayed so that we can have only one message to be processed
     * in the queue.
     */
    private void removeAndSendMessageDelayedToHandler() {
        checkInjectorRuntimeAvailabilityHandler.removeMessages(MESSAGE_WHAT);
        checkInjectorRuntimeAvailabilityHandler.sendEmptyMessageDelayed(MESSAGE_WHAT, DELAY_MILLISECOND_FOR_RETRY_CHECK_CC);
    }
}
