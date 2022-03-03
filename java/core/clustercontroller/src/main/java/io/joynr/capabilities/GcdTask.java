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
        ADD, REMOVE, RE_ADD
    }

    static abstract class CallbackCreator {
        public abstract CallbackWithModeledError<Void, DiscoveryError> createCallback();
    }

    private final MODE mode;
    private final CallbackCreator callbackCreator;
    private final String participantId;
    private final GlobalDiscoveryEntry globalDiscoveryEntry;
    private final String[] gbids;
    private final long expiryDateMs;
    private final boolean doRetry;

    public MODE getMode() {
        return mode;
    }

    public CallbackCreator getCallbackCreator() {
        return callbackCreator;
    }

    public String getParticipantId() {
        return participantId;
    }

    public GlobalDiscoveryEntry getGlobalDiscoveryEntry() {
        return globalDiscoveryEntry;
    }

    public String[] getGbids() {
        return gbids;
    }

    public long getExpiryDateMs() {
        return expiryDateMs;
    }

    public boolean isDoRetry() {
        return doRetry;
    }

    protected GcdTask(MODE mode,
                      CallbackCreator callbackCreator,
                      String participantId,
                      GlobalDiscoveryEntry globalDiscoveryEntry,
                      String[] gbids,
                      long expiryDateMs,
                      boolean doRetry) {
        this.mode = mode;
        this.callbackCreator = callbackCreator;
        this.participantId = participantId;
        this.globalDiscoveryEntry = globalDiscoveryEntry;
        this.gbids = gbids;
        this.expiryDateMs = expiryDateMs;
        this.doRetry = doRetry;
    }

    public static GcdTask createAddTask(CallbackCreator callbackCreator,
                                        GlobalDiscoveryEntry globalDiscoveryEntry,
                                        long expiryDateMs,
                                        String[] gbids,
                                        boolean doRetry) {

        String participantId = "";
        return new GcdTask(MODE.ADD,
                           callbackCreator,
                           participantId,
                           globalDiscoveryEntry,
                           gbids,
                           expiryDateMs,
                           doRetry);
    }

    public static GcdTask createReaddTask() {
        String participantId = "";
        GlobalDiscoveryEntry globalDiscoveryEntry = null;
        String[] gbids = null;
        long expiryDateMs = 0L;
        return new GcdTask(MODE.RE_ADD, null, participantId, globalDiscoveryEntry, gbids, expiryDateMs, false);
    }

    public static GcdTask createRemoveTask(CallbackCreator callbackCreator, String participantId) {
        GlobalDiscoveryEntry globalDiscoveryEntry = null;
        long expiryDateMs = 0L;
        String[] gbids = null;
        return new GcdTask(MODE.REMOVE,
                           callbackCreator,
                           participantId,
                           globalDiscoveryEntry,
                           gbids,
                           expiryDateMs,
                           true);
    }
}
