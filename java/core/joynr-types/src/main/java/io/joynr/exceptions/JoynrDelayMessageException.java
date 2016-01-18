package io.joynr.exceptions;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

public class JoynrDelayMessageException extends JoynrRuntimeException {
    private static final long serialVersionUID = 1L;

    private long delayMs;

    /**
     *
     * @param delayMs how long the message should be delayed
     * @param reason why the message is being delayed
     */
    public JoynrDelayMessageException(long delayMs, String reason) {
        super(reason);
        this.delayMs = delayMs;
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
