package io.joynr.messaging.service;

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

import io.joynr.messaging.info.BounceProxyStatus;

/**
 * Helper class to allow non-case sensitive parsing of {@link BounceProxyStatus}
 * parameters.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyStatusParam {

    private BounceProxyStatus status;

    /**
     * The status that was passed in as parameter.
     */
    private String passedInStatus;

    /**
     * Creates a bounce proxy status parameter from a string.
     * 
     * @param status
     *            a status parameter as string. If it is one of the
     *            {@link io.joynr.messaging.info.BounceProxyStatus} enumeration values (case insensitiv)
     *            except {@link io.joynr.messaging.info.BounceProxyStatus#UNRESOLVED}, then this is set
     *            as internal status. If <code>null</code>, the internal
     *            {@link io.joynr.messaging.info.BounceProxyStatus} is <code>null</code>, too. If a
     *            string is passed in that won't resolve to any of
     *            {@link io.joynr.messaging.info.BounceProxyStatus} except
     *            {@link io.joynr.messaging.info.BounceProxyStatus#UNRESOLVED}, then
     *            {@link io.joynr.messaging.info.BounceProxyStatus#UNRESOLVED} is set.
     */
    public BounceProxyStatusParam(String status) {
        this.passedInStatus = status;
        try {
            if (status == null) {
                this.status = null;
            } else {
                this.status = BounceProxyStatus.valueOf(status.toUpperCase());
            }
        } catch (IllegalArgumentException e) {
            // is thrown if there's no BounceProxyStatus with status as name
            this.status = BounceProxyStatus.UNRESOLVED;
        }
    }

    /**
     * Returns the wrapped {@link io.joynr.messaging.info.BounceProxyStatus}.
     * 
     * @return the bounce proxy status {@link io.joynr.messaging.info.BounceProxyStatus}
     */
    public BounceProxyStatus getStatus() {
        return this.status;
    }

    /**
     * Returns the parameter that was passed in to create a
     * {@link io.joynr.messaging.info.BounceProxyStatus} object that is returned by {@link #getStatus()}
     * .
     * 
     * @return e parameter that was passed in to create a
     * {@link io.joynr.messaging.info.BounceProxyStatus} object that is returned by {@link #getStatus()}
     */
    public String getPassedInStatus() {
        return this.passedInStatus;
    }
}
