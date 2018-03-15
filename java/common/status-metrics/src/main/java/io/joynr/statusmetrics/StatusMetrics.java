/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

public interface StatusMetrics {
    /**
     * Provides status information for message workers. Message workers consume messages from the
     * message queue and process them. Evaluating their status allows to detect states in which
     * a thread blocks indefinitely.
     *
     * @see MessageWorkerStatus for more information.
     *
     * @return Returns an array which contains the status of each message worker.
     */
    MessageWorkerStatus[] getMessageWorkersStatus();
}