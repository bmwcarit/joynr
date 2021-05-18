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

public class GcdTask {
    public enum MODE {
        ADD, REMOVE, READD
    }

    private GcdTask(MODE mode,
                    CallbackWithModeledError<Void, DiscoveryError> callback,
                    String participantId,
                    GlobalDiscoveryEntry globalDiscoveryEntry,
                    String[] gbids,
                    long expiryDateMs,
                    boolean doRetry) {
        this.mode = mode;
        this.callback = callback;
        this.participantId = participantId;
        this.globalDiscoveryEntry = globalDiscoveryEntry;
        this.gbids = gbids;
        this.expiryDateMs = expiryDateMs;
        this.doRetry = doRetry;
    }

    public static GcdTask createAddTask(CallbackWithModeledError<Void, DiscoveryError> callback,
                                        GlobalDiscoveryEntry globalDiscoveryEntry,
                                        long expiryDateMs,
                                        String[] gbids,
                                        boolean doRetry) {

        String participantId = "";
        return new GcdTask(MODE.ADD, callback, participantId, globalDiscoveryEntry, gbids, expiryDateMs, doRetry);
    }

    public static GcdTask createReaddTask() {
        CallbackWithModeledError<Void, DiscoveryError> callback = null;
        String participantId = "";
        GlobalDiscoveryEntry globalDiscoveryEntry = null;
        String[] gbids = null;
        long expiryDateMs = 0L;
        return new GcdTask(MODE.READD, callback, participantId, globalDiscoveryEntry, gbids, expiryDateMs, false);
    }

    public static GcdTask createRemoveTask(CallbackWithModeledError<Void, DiscoveryError> callback,
                                           String participantId) {
        GlobalDiscoveryEntry globalDiscoveryEntry = null;
        long expiryDateMs = 0L;
        String[] gbids = null;
        return new GcdTask(MODE.REMOVE, callback, participantId, globalDiscoveryEntry, gbids, expiryDateMs, true);
    }

    public final MODE mode;
    public final CallbackWithModeledError<Void, DiscoveryError> callback;
    public final String participantId;
    public final GlobalDiscoveryEntry globalDiscoveryEntry;
    public final String[] gbids;
    public final long expiryDateMs;
    public final boolean doRetry;
}
