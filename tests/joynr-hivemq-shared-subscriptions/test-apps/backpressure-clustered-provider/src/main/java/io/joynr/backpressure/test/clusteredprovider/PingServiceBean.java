/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.backpressure.test.clusteredprovider;

import java.net.InetAddress;
import java.net.UnknownHostException;

import jakarta.ejb.Stateless;
import com.google.inject.Inject;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.sharedsubscription.test.clusteredapp.base.CallStatistics;
import joynr.io.joynr.sharedsubscriptions.test.PingServiceSync;

/**
 * Implementation of the {@link PingServiceSync ping service}.
 */
@Stateless
@ServiceProvider(serviceInterface = PingServiceSync.class)
public class PingServiceBean implements PingServiceSync {

    private CallStatistics callStatistics;

    @Inject
    public PingServiceBean(CallStatistics callStatistics) {
        this.callStatistics = callStatistics;
    }

    @Override
    public String ping() {
        // make computation slow
        try {
            Thread.sleep(50);
        } catch (InterruptedException e1) {
            e1.printStackTrace();
        }
        callStatistics.ping();
        try {
            return InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            return System.getProperty("java.io.tmpdir");
        }
    }

}
