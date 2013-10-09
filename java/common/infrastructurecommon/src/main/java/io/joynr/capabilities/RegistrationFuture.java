package io.joynr.capabilities;

/*
 * #%L
 * joynr::java::common::infrastructurecommon
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import java.util.Date;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class RegistrationFuture {

    // The current status of the registration
    private RegistrationStatus status;

    // The participant ID of the registered provider
    private String participantId;

    // A lock that must be acquired before reading or writing the status
    private final Lock statusLock;

    // A condition used by threads to wait on a status change
    private final Condition statusChanged;

    /**
     * Create a new Future that assumes that local registration is already in progress
     */
    public RegistrationFuture(String participantId) {
        this(RegistrationStatus.REGISTERING_LOCALLY, participantId);
    }

    /**
     * Create a new Future with the given status
     */
    public RegistrationFuture(RegistrationStatus status, String participantId) {
        this.status = status;
        this.participantId = participantId;
        statusLock = new ReentrantLock();
        statusChanged = statusLock.newCondition();
    }

    /**
     * Get the current status of the registration
     * @return The status of the registration
     */
    public RegistrationStatus getStatus() {
        statusLock.lock();
        try {
            return status;
        } finally {
            statusLock.unlock();
        }
    }

    /**
     * Set the current status of the registration
     */
    public void setStatus(RegistrationStatus status) {
        statusLock.lock();
        try {
            this.status = status;
            statusChanged.signal();
        } finally {
            statusLock.unlock();
        }
    }

    /**
     * Wait for registration with the local capabilities directory
     * @param timeout_ms The timeout in milliseconds
     * @return true if the registration completed successfully before the timeout
     * @throws InterruptedException If the thread is interrupted
     */
    public boolean waitForLocalRegistration(long timeout_ms) throws InterruptedException {
        try {
            statusLock.lock();
            if (this.status == RegistrationStatus.REGISTERING_LOCALLY) {
                if (!statusChanged.await(timeout_ms, TimeUnit.MILLISECONDS)) {
                    return false;
                }
            }
            return (this.status != RegistrationStatus.ERROR);
        } finally {
            statusLock.unlock();
        }
    }

    /**
     * Waits for registration to complete.
     * Usually this method waits for registration to complete on both the local and the global capabilities directories.
     * If the registration is localOnly this method only waits for the local directory.
     * @param timeout_ms The timeout in milliseconds
     * @return true if the registration completed successfully before the timeout
     * @throws InterruptedException If the thread is interrupted
     */
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "SF_SWITCH_FALLTHROUGH", justification = "fall through is deliberate and commented")
    public boolean waitForFullRegistration(long timeout_ms) throws InterruptedException {
        try {
            long timeLeft = timeout_ms;
            statusLock.lock();
            switch (this.status) {
            case REGISTERING_LOCALLY:
                Date start = new Date();
                if (!statusChanged.await(timeLeft, TimeUnit.MILLISECONDS)) {
                    return false;
                }
                if (this.status == RegistrationStatus.ERROR) {
                    return false;
                }
                // Local only registration will be finished at this point
                if (this.status == RegistrationStatus.DONE) {
                    return true;
                }
                Date end = new Date();
                timeLeft -= end.getTime() - start.getTime();
                // fall through
            case REGISTERING_GLOBALLY:
                if (!statusChanged.await(timeLeft, TimeUnit.MILLISECONDS)) {
                    return false;
                }
                // fall through
            default:
                return (this.status == RegistrationStatus.DONE);
            }
        } finally {
            statusLock.unlock();
        }
    }

    /**
     * @return The participant ID assigned to the registered provider.
     */
    public String getParticipantId() {
        return participantId;
    }

}
