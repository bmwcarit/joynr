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
package io.joynr.qos;

public class QualityOfService {

    private static final long defaultCacheEntryTimeToLiveMs = 86400000; // 24 hours
    private static final long defaultDataFreshnessMs = 600000; // 10 minutes

    private long cacheEntryTimeToLiveMs = defaultCacheEntryTimeToLiveMs;
    private long dataFreshnessMs = defaultDataFreshnessMs;

    public QualityOfService() {
    }

    /**
     * Copy constructor
     *
     * @param other reference to the object to be copied
     */
    public QualityOfService(QualityOfService other) {
        this.dataFreshnessMs = other.dataFreshnessMs;
        this.cacheEntryTimeToLiveMs = other.getCacheTimeToLiveMs();
    }

    public void setCacheTimeToLiveMs(long milliseconds) {
        this.cacheEntryTimeToLiveMs = milliseconds;
    }

    public Long getCacheTimeToLiveMs() {
        return cacheEntryTimeToLiveMs;
    }

    public void setDataFreshnessMs(long milliseconds) {
        this.dataFreshnessMs = milliseconds;
    }

    public Long getDataFreshnessMs() {
        return this.dataFreshnessMs;
    }
}
