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
package io.joynr.capabilities;

import io.joynr.proxy.CallbackWithModeledError;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

public class GlobalAddRemoveQueueEntry {
    public enum MODE {
        ADD, REMOVE
    }

    public GlobalAddRemoveQueueEntry(CallbackWithModeledError<Void, DiscoveryError> callback,
                                     GlobalDiscoveryEntry globalDiscoveryEntry,
                                     String[] gbids) {
        mode = MODE.ADD;
        this.callback = callback;
        this.globalDiscoveryEntry = globalDiscoveryEntry;
        this.gbids = gbids;
        this.participantId = "";
    }

    public GlobalAddRemoveQueueEntry(CallbackWithModeledError<Void, DiscoveryError> callback, String participantId) {
        mode = MODE.REMOVE;
        this.callback = callback;
        this.participantId = participantId;
        this.globalDiscoveryEntry = null;
        this.gbids = null;
    }

    public final MODE mode;
    public final CallbackWithModeledError<Void, DiscoveryError> callback;
    public final String participantId;
    public final GlobalDiscoveryEntry globalDiscoveryEntry;
    public final String[] gbids;
}
