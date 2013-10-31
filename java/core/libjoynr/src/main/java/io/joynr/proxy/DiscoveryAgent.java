package io.joynr.proxy;

/*
 * #%L
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

import io.joynr.arbitration.ArbitrationCallback;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.ArbitrationStatus;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class DiscoveryAgent implements ArbitrationCallback {

    public ArbitrationStatus arbitrationStatus;
    private Lock arbitrationStatusLock = new ReentrantLock();
    private ArbitrationResult arbitrationResult;
    private ProxyInvocationHandler proxyInvocationHandler;

    public DiscoveryAgent() {
        arbitrationStatus = ArbitrationStatus.ArbitrationNotStarted;

    }

    /**
     * notifyArbitrationStatusChanged should be called by the arbitrator when
     * the arbitrationStatus changes but the result is not yet available / not
     * valid.
     */
    @Override
    public void notifyArbitrationStatusChanged(ArbitrationStatus notifiedArbitrationStatus) {
        arbitrationStatusLock.lock();
        try {
            this.arbitrationStatus = notifiedArbitrationStatus;
        } finally {
            arbitrationStatusLock.unlock();
        }
    }

    /**
     * setArbitrationResult should be called by the arbitrator when a valid
     * arbitrationResult is available.
     */
    @Override
    public void setArbitrationResult(ArbitrationStatus arbitrationStatus, ArbitrationResult arbitrationResult) {
        arbitrationStatusLock.lock();
        try {
            this.arbitrationResult = arbitrationResult;
            this.arbitrationStatus = arbitrationStatus;
            if (arbitrationStatus == ArbitrationStatus.ArbitrationSuccesful) {
                proxyInvocationHandler.createConnector(arbitrationResult);
            }
        } finally {
            arbitrationStatusLock.unlock();
        }

    }

    /**
     * This method should be called by the ProxyBuilder when creating the
     * ProxyInvocationHandler Is is necessary because the DiscoveryAgents needs
     * to set the connector for this ProxyInvocationHandler
     */
    public void setProxyInvocationHandler(ProxyInvocationHandler proxyInvocationHandler) {
        this.proxyInvocationHandler = proxyInvocationHandler;

    }

    public ArbitrationResult getArbitrationResult() {
        return arbitrationResult;
    }

}
