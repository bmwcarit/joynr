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
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.PerformanceMeasures;

import java.util.List;

/**
 * Interface for monitoring service implementation.
 *
 * @author christina.strobel
 *
 */
public interface MonitoringService {

    /**
     * Returns a list of registered bounce proxies.
     *
     * @return list of registered bounce proxies
     */
    public List<BounceProxyStatusInformation> getRegisteredBounceProxies();

    /**
     * Registers a new bounce proxy.
     *
     * @param bpId
     *            the ID of the bounce proxy
     * @param urlForCc
     *            the base URL at which the bounce proxy will be reachable for
     *            cluster controllers
     * @param urlForBpc
     *            the base URL at which the bounce proxy will be reachable for
     *            the bounce proxy controller
     */
    void register(String bpId, String urlForCc, String urlForBpc);

    /**
     * Updates a bounce proxy. This will update urls in case they changed after a
     * new startup and will reset monitoring measures as well as the bounce
     * proxy status. After executing this method, subsequent channel assignments
     * have to take into account that any status measures for this bounce proxy
     * are the results of monitoring reports before shutdown or crash of the
     * bounce proxy.
     *
     * @param bpId
     *            the ID of the bounce proxy
     * @param urlForCc
     *            the base URL at which the bounce proxy will be reachable for
     *            cluster controllers
     * @param urlForBpc
     *            the base URL at which the bounce proxy will be reachable for
     *            the bounce proxy controller
     */
    void update(String bpId, String urlForCc, String urlForBpc);

    /**
     * Updates performance measures for a bounce proxy.
     *
     * @param bpId
     *            the ID of the bounce proxy
     * @param performanceMeasures
     *            several performance measures for the bounce proxy
     */
    void updatePerformanceMeasures(String bpId, PerformanceMeasures performanceMeasures);

    /**
     * Updates status for a bounce proxy.
     *
     * @param bpId
     *            the ID of the bounce proxy
     * @param status
     *            the status of a bounce proxy
     */
    void updateStatus(String bpId, BounceProxyStatus status);

    /**
     * Returns if a bounce proxy instance with the same ID has already been
     * registered.
     *
     * @param bpId the ID of the bounce proxy
     * @return {@literal <code>true</code>} if there's already a bounce proxy
     * instance registered with this ID or {@literal <code><code>false</code>}
     * if not.
     */
    boolean isRegistered(String bpId);

}
