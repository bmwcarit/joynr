package io.joynr.caching;

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

import io.joynr.qos.TimeComparisonCompatibility;

public class CachedValue implements TimeComparisonCompatibility {

    private final Object value;
    private Long timestamp;

    public CachedValue(Object value, Long timestamp) {
        this.value = value;
        this.timestamp = timestamp;
    }

    public Object getValue() {
        return value;
    }

    @Override
    public Long getTimeInMilliSec() {
        return timestamp;
    }
}
