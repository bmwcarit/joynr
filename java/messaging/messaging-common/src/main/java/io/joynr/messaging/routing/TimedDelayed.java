package io.joynr.messaging.routing;

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

import static java.util.concurrent.TimeUnit.NANOSECONDS;

import java.util.concurrent.Delayed;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

public class TimedDelayed implements Delayed {
    static final AtomicLong sequencer = new AtomicLong();
    protected long delayUntilDate;
    protected long sequenceNumber;

    public TimedDelayed() {
        super();
    }

    public TimedDelayed(long delayForMs) {
        this.delayUntilDate = System.currentTimeMillis() + delayForMs;
        this.sequenceNumber = sequencer.getAndIncrement();
    }

    @Override
    public int compareTo(Delayed other) {
        if (other == this) {
            // equal only if the same object
            return 0;
        }

        long diff = getDelay(NANOSECONDS) - other.getDelay(NANOSECONDS);
        if (diff < 0) {
            return -1;
        } else if (diff > 0) {
            return 1;
        } else if (other instanceof TimedDelayed) {
            TimedDelayed otherTimedDelayed = (TimedDelayed) other;
            return (sequenceNumber < otherTimedDelayed.sequenceNumber) ? -1 : 1;
        } else {
            return 0;
        }
    }

    @Override
    public long getDelay(TimeUnit unit) {
        long diff = delayUntilDate - System.currentTimeMillis();
        return unit.convert(diff, TimeUnit.MILLISECONDS);
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (delayUntilDate ^ (delayUntilDate >>> 32));
        result = prime * result + (int) (sequenceNumber ^ (sequenceNumber >>> 32));
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        TimedDelayed other = (TimedDelayed) obj;
        if (delayUntilDate != other.delayUntilDate)
            return false;
        if (sequenceNumber != other.sequenceNumber)
            return false;
        return true;
    }
}