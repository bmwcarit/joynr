/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.statusmetrics;

import java.util.Collection;

/**
 * Can be injected to receive status metrics from joynr. These metrics are useful to monitor the state of a service.
 */
public interface JoynrStatusMetrics {
    /**
     * @return Returns an unfiltered collection with all status metrics objects.
     */
    Collection<ConnectionStatusMetrics> getAllConnectionStatusMetrics();

    /**
     * @return Returns a collection with the status metrics objects for the given gbid.
     * @param gbid the selected GBID
     */
    Collection<ConnectionStatusMetrics> getConnectionStatusMetrics(String gbid);

    /**
     * @return Returns the number of request messages which were discarded because the message queue reached its upper limit.
     */
    long getNumDroppedMessages();
}
