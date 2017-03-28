package io.joynr.messaging.bounceproxy.monitoring;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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
 * Interface to monitor the lifecycle of a bounce proxy.
 * 
 * @author christina.strobel
 * 
 */
public interface BounceProxyLifecycleMonitor {

    /**
     * Returns whether a bounce proxy is initialized, i.e. fully functional.
     * 
     * @return <code>true</code> if the bounce proxy is fully functional due to
     *         proper initialization, <code>false</code> if e.g. one of these
     *         happened:
     *         <ul>
     *         <li>the bounce proxy could not register with the bounce proxy
     *         controller</li>
     *         <li>the bounce proxy could not initialize its local resource
     *         properly</li>
     *         <li>to be continued...</li>
     *         </ul>
     */
    public boolean isInitialized();

}
