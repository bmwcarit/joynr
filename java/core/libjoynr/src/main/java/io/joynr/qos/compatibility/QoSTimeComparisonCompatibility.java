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
package io.joynr.qos.compatibility;

import java.util.function.Predicate;

import io.joynr.qos.QualityOfService;
import io.joynr.qos.TimeComparisonCompatibility;

public abstract class QoSTimeComparisonCompatibility implements Predicate<TimeComparisonCompatibility> {

    protected final QualityOfService qos;

    public QoSTimeComparisonCompatibility(QualityOfService qos) {
        this.qos = qos;
    }

    @Override
    public boolean test(TimeComparisonCompatibility input) {
        Long ttl = getQoSTimeInMillis();
        long currentTime = System.currentTimeMillis();
        long delta = currentTime - input.getTimeInMilliSec();
        return condition(ttl, delta);
    }

    protected abstract boolean condition(long delta, long qosTimeConstraint);

    protected abstract Long getQoSTimeInMillis();
}
