package io.joynr.messaging.bounceproxy.filter;

import joynr.ImmutableMessage;

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

import org.atmosphere.cpr.BroadcastFilter;
import org.atmosphere.cpr.BroadcastFilter.BroadcastAction.ACTION;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ExpirationFilter implements BroadcastFilter {

    private static final Logger log = LoggerFactory.getLogger(ExpirationFilter.class);

    @Override
    public BroadcastAction filter(Object originalMessage, Object message) {
        if (inspect(message)) {
            return new BroadcastAction(ACTION.CONTINUE, message);
        }

        return new BroadcastAction(ACTION.ABORT, message);
    }

    public boolean inspect(Object message) {
        long expirationDate = 0;

        if (message instanceof ImmutableMessage) {
            ImmutableMessage immutableMessage = (ImmutableMessage) message;

            assert (immutableMessage.isTtlAbsolute());
            expirationDate = immutableMessage.getTtlMs();
            if (expirationDate < System.currentTimeMillis()) {
                log.warn("message expired: msgId: {}", immutableMessage.getId());
                return false;
            }
            log.trace("message has not expired: msgId: {}", immutableMessage.getId());
            return true;
        }

        log.warn("could not filter message NOT correct type {}", message);
        // let everything else pass
        return true;
    }
}
