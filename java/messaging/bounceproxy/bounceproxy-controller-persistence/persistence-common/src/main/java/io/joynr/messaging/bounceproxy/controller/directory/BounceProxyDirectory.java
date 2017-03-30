package io.joynr.messaging.bounceproxy.controller.directory;

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

import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatusInformation;

import java.util.List;

/**
 * The directory stores all bounce proxy instances that are registered with the
 * bounce proxy controller.
 *
 * @author christina.strobel
 *
 */
public interface BounceProxyDirectory {

    /**
     * Returns all bounce proxy instances that have the right status to take new
     * channels. I.e. they must not be in a shutdown or excluded state.
     *
     * @return list of bound proxy instances that have the right status to
     * take new channels.
     */
    public List<BounceProxyRecord> getAssignableBounceProxies();

    /**
     * Updates the channel assignment for a bounce proxy, i.e. registers that a
     * channel was assigned to the bounce proxy instance.
     *
     * If there's no directory entry for the bounce proxy, the call is simply
     * ignored without warning. Make sure before calling this method that the
     * bounce proxy is registered.
     *
     * @param ccid the channel id
     * @param bpInfo the bounce proxy information
     *
     * @throws IllegalArgumentException
     *             if no bounce proxy with this ID is registered in the
     *             directory or if no channel with ccid is registered in the {@link ChannelDirectory}.
     */
    public void updateChannelAssignment(String ccid, BounceProxyInformation bpInfo) throws IllegalArgumentException;

    /**
     * Gets a record of a bounce proxy. Before calling this method, it should be
     * checked with {@link #containsBounceProxy(String)} if a bounce proxy with
     * this ID has been added to the directory.
     *
     * @param bpId
     *            the identifier of the bounce proxy
     * @return a bounce proxy record with this ID
     * @throws IllegalArgumentException
     *             if no bounce proxy with this ID is registered in the
     *             directory
     */
    public BounceProxyRecord getBounceProxy(String bpId) throws IllegalArgumentException;

    /**
     * Checks whether a certain bounce proxy is registered in the directory.
     *
     * @param bpId
     *            the identifier of the bounce proxy
     * @return <code>true</code> if there is a record for this bounce proxy,
     *         <code>false</code> if it never has been registered.
     */
    public boolean containsBounceProxy(String bpId);

    /**
     * Adds a new bounce proxy that hasn't been registered before to the
     * directory. Before adding a new bounce proxy, it should be checked with
     * {@link #containsBounceProxy(String)} whether a bounce proxy with this
     * identifier is already registered.
     *
     * @param bpInfo
     *            information about the bounce proxy.
     * @throws IllegalArgumentException
     *             if the bounce proxy has already been registered before
     */
    public void addBounceProxy(ControlledBounceProxyInformation bpInfo) throws IllegalArgumentException;

    /**
     * Updates the record about an existing bounce proxy. The bounce proxy
     * record to be updated should be retrieved by
     * {@link #getBounceProxy(String)} before.
     *
     * @param bpRecord
     *            the updated record of a bounce proxy
     * @throws IllegalArgumentException
     *             if no bounce proxy with the same ID is registered in the
     *             directory
     */
    public void updateBounceProxy(BounceProxyRecord bpRecord) throws IllegalArgumentException;

    /**
     * Returns the list of registered bounce proxies including information such
     * as performance measures, freshness and status.
     *
     * @return list of registered bounce proxies including information such
     * as performance measures, freshness and status.
     */
    public List<BounceProxyStatusInformation> getBounceProxyStatusInformation();
}
