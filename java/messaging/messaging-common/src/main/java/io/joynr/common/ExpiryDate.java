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
package io.joynr.common;

import java.text.DateFormat;
import java.text.SimpleDateFormat;

/**
 * Instances of this class represent absolute time stamps since 01 Jan 1970 00:00:00 .
 * 
 */

public class ExpiryDate {

    private final long value;
    private final long relativeTtl;

    /**
     * 
     * @param relativeTtl milliseconds until the ttl expires
     * @return an ExpiryDate with creationTime = current time and value = creationTime + relativeTtl
     */
    public static ExpiryDate fromRelativeTtl(final long relativeTtl) {
        final long creationTime = System.currentTimeMillis();
        final long expiryDate = ExpiryDateUtils.addAndLimit(creationTime, relativeTtl);
        return new ExpiryDate(relativeTtl, expiryDate);
    }

    /**
     * NOTE: relative Ttl can be negative if the ExpiryDate passed in was in the past
     * @param expiryDate time measured in milliseconds, between the current time and midnight, January 1, 1970, UTC
     * @return an ExpiryDate object with creationTime = current time, and relativeTtl = ExpiryDate - creationTime;
     */
    public static ExpiryDate fromAbsolute(final long expiryDate) {
        final long creationTime = System.currentTimeMillis();
        final long relativeTtl = ExpiryDateUtils.subtract(expiryDate, creationTime);
        return new ExpiryDate(relativeTtl, expiryDate);
    }

    /**
     * 
     * @param relativeTtl the absolute time will be set based on this value relative to the point in time when the object is created
     */
    private ExpiryDate(final long relativeTtl, final long expiryDate) {
        this.relativeTtl = relativeTtl;
        this.value = expiryDate;
    }

    public long getValue() {
        return value;
    }

    /**
     * 
     * @return the original relative ttl used to create this object
     */
    public long getRelativeTtl() {
        return relativeTtl;
    }

    @Override
    public String toString() {
        final DateFormat dateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
        return "ExpiryDate in ms: " + value + " ExpiryDate: " + dateFormatter.format(value) + " relativeTtl:"
                + relativeTtl;
    }
}
