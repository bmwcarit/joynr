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
package io.joynr.exceptions;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class JoynrDelayMessageException extends JoynrRuntimeException {
    private static final long serialVersionUID = 1L;

    private static final long DEFAULT_DELAY_MS = 3000;

    //TODO should use @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS)
    @Inject(optional = true)
    @Named("joynr.messaging.sendmsgretryintervalms")
    private long delayMs = DEFAULT_DELAY_MS;

    /**
     * Uses default delay as injected using property joynr.messaging.sendmsgretryintervalms
     * or 1 sec if no value was injected.
     *
     * @param reason why the message is being delayed
     */
    public JoynrDelayMessageException(String reason) {
        super(reason);
    }

    /**
     *
     * @param delayMs how long the message should be delayed
     * @param reason why the message is being delayed
     */
    public JoynrDelayMessageException(long delayMs, String reason) {
        this(reason);
        this.delayMs = delayMs;
    }

    /**
     *
     * @param delayMs how long the message should be delayed
     * @param reason why the message is being delayed
     */
    public JoynrDelayMessageException(long delayMs, String reason, Throwable cause) {
        super(reason, cause);
        this.delayMs = delayMs;
    }

    public JoynrDelayMessageException(String reason, Throwable cause) {
        super(reason, cause);
    }

    public long getDelayMs() {
        return delayMs;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + (int) (delayMs ^ (delayMs >>> 32));
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        JoynrDelayMessageException other = (JoynrDelayMessageException) obj;
        if (delayMs != other.delayMs) {
            return false;
        }
        return true;
    }
}
