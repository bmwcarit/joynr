package io.joynr.messaging.bounceproxy.controller.directory;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.messaging.bounceproxy.controller.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;

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
     * @return
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
     * @param ccid
     * @param bpInfo
     */
    public void updateChannelAssignment(String ccid, BounceProxyInformation bpInfo);

    public BounceProxyRecord getBounceProxy(String bpId);

    public void addBounceProxy(ControlledBounceProxyInformation bpInfo);

    public void updateBounceProxyStatus(String bpId, BounceProxyStatus status);

    public List<String> getBounceProxyIds();

}
