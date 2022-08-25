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

import com.google.inject.Inject;
import com.google.inject.Injector;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.inject.Named;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.android.messaging.binder.JoynrBinder;
import joynr.ImmutableMessage;

public class BinderMessagingSkeleton extends JoynrBinder.Stub {

    public final static String BINDER_IS_MAIN_TRANSPORT = "io.joynr.android.binder.is.main.transport";

    private static final Logger logger = LoggerFactory.getLogger(BinderMessagingSkeleton.class);

    private BinderMessageProcessor binderMessageProcessor;

    private boolean mainTransport;

    public BinderMessagingSkeleton() {
        /*
         * TODO: For now, all messages on CC or Libjoynr have MainTransportFlagBearer as TRUE
         *  we need to implement the Guice injection so that this is only true on the
         *  Libjoynr side
         */
        this.mainTransport = true;
        this.binderMessageProcessor = new BinderMessageProcessor(true);
    }

    @Inject
    BinderMessagingSkeleton(MainTransportFlagBearer mainTransportFlagBearer) {
        this.mainTransport = mainTransportFlagBearer.isMainTransport();
        this.binderMessageProcessor = new BinderMessageProcessor(this.mainTransport);
        logger.info("MAIN TRANSPORT: " + this.mainTransport);
    }

    protected void setBinderMessageProcessor(BinderMessageProcessor binderMessageProcessor) {
        this.binderMessageProcessor = binderMessageProcessor;
    }

    @Override
    public void transmit(byte[] serializedMessage) {

        try {
            ImmutableMessage message = new ImmutableMessage(serializedMessage);

            logger.debug("BinderService <<< INCOMING <<<< {}", message);
            Injector injector = AndroidBinderRuntime.getInjector();
            if (injector != null) {
                binderMessageProcessor.processMessage(injector, message);
            } else {
                binderMessageProcessor.addToWaitingProcessing(message);
                binderMessageProcessor.removeAndSendMessageDelayedToHandler();
            }

        } catch (Exception error) {
            logger.error("BinderService transmit message not processed: {}", error.getMessage());
        }
    }

    public static class MainTransportFlagBearer {
        @Inject(optional = true)
        @Named(BINDER_IS_MAIN_TRANSPORT)
        private Boolean mainTransport;

        public MainTransportFlagBearer() {
        }

        protected MainTransportFlagBearer(Boolean mainTransport) {
            this.mainTransport = mainTransport;
        }

        public boolean isMainTransport() {
            return Boolean.TRUE.equals(mainTransport);
        }
    }
}
