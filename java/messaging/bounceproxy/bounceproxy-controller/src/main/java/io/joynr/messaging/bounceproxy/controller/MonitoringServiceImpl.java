package io.joynr.messaging.bounceproxy.controller;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.PerformanceMeasures;
import io.joynr.messaging.service.MonitoringService;

import java.net.URI;
import java.util.List;

import com.google.inject.Inject;

/**
 * Implementation of bounce proxy controller for monitoring service.
 * 
 * @author christina.strobel
 * 
 */
public class MonitoringServiceImpl implements MonitoringService {

    @Inject
    private BounceProxyDirectory bounceProxyDirectory;

    @Override
    public List<BounceProxyStatusInformation> getRegisteredBounceProxies() {
        return bounceProxyDirectory.getBounceProxyStatusInformation();
    }

    @Override
    public void register(String bpId, String urlForCc, String urlForBpc) {

        if (!bounceProxyDirectory.containsBounceProxy(bpId)) {
            ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation(bpId,
                                                                                           URI.create(urlForCc),
                                                                                           URI.create(urlForBpc));
            bounceProxyDirectory.addBounceProxy(bpInfo);
        }
    }

    @Override
    public void update(String bpId, String urlForCc, String urlForBpc) {

        BounceProxyRecord bounceProxyRecord = bounceProxyDirectory.getBounceProxy(bpId);
        bounceProxyRecord.getInfo().setLocation(URI.create(urlForCc));
        bounceProxyRecord.getInfo().setLocationForBpc(URI.create(urlForBpc));
        bounceProxyRecord.setStatus(BounceProxyStatus.ALIVE);
        bounceProxyDirectory.updateBounceProxy(bounceProxyRecord);
    }

    @Override
    public void updatePerformanceMeasures(String bpId, PerformanceMeasures performanceMeasures) {

        BounceProxyRecord bounceProxyRecord = bounceProxyDirectory.getBounceProxy(bpId);
        bounceProxyRecord.setPerformanceMeasures(performanceMeasures);
        bounceProxyRecord.setStatus(BounceProxyStatus.ACTIVE);
        bounceProxyDirectory.updateBounceProxy(bounceProxyRecord);
    }

    @Override
    public void updateStatus(String bpId, BounceProxyStatus status) {

        BounceProxyRecord bounceProxyRecord = bounceProxyDirectory.getBounceProxy(bpId);
        bounceProxyRecord.setStatus(status);
        bounceProxyDirectory.updateBounceProxy(bounceProxyRecord);
    }

    @Override
    public boolean isRegistered(String bpId) {
        return bounceProxyDirectory.containsBounceProxy(bpId);
    }

}
