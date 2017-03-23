package io.joynr.messaging.info;

/*
 * #%L
 * joynr::java::messaging::bounceproxy-controller-service
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

/**
 * Status indication for bounce proxy which is stored by the controller.
 * 
 * @author christina.strobel
 * 
 */
public enum BounceProxyStatus {

    /**
     * The bounce proxy has just been started or resetted and waits for channels
     * to be assigned. All performance measures stored for this bounce proxy
     * arise from reports before the bounce proxy was started or reset, and
     * cannot be relied on.
     */
    ALIVE,

    /**
     * The bounce proxy has channels assigned and performance measures are up to
     * date.
     */
    ACTIVE,

    /**
     * The bounce proxy is currently shut down, thus no new channels should be
     * assigned to it. It waits for a reset.
     */
    SHUTDOWN,

    /**
     * The bounce proxy is being migrated to a different server instance and
     * can't take over new channels.
     */
    EXCLUDED,

    /**
     * The bounce proxy reported a status that could not be resolved to any of
     * the other statuses and thus can't be processed.
     */
    UNRESOLVED;

    /**
     * Returns whether the bounce proxy is in a status in which it can handle
     * new channels.
     * 
     * @return <code>true</code> if new channels can be assigned to a bounce
     *         proxy, <code>false</code> otherwise
     */
    public boolean isAssignable() {
        return this == ALIVE || this == ACTIVE;
    }

    /**
     * Checks whether a transition from one status to another one is valid.
     * 
     * @param status
     *            the new status
     * @return <code>true</code> if the transition is valid, <code>false</code>
     *         otherwise
     */
    public boolean isValidTransition(BounceProxyStatus status) {

        // Invalid transitions are very rare as this is a simply state machine
        // for which almost all transitions are valid. Thus this is not
        // implemented with the state pattern but only with a simple check.

        if (this == status) {
            // transition to itself
            return true;
        }

        if (this == EXCLUDED || this == UNRESOLVED) {
            // these states can't be left without administrator interaction
            return false;
        }

        if (this == SHUTDOWN && status != ALIVE) {
            // can only go to alive from shutdown
            return false;
        }

        return true;
    }
}
