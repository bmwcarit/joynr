package io.joynr.messaging.info;

import java.util.Date;

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
 * Aggregates information about a bounce proxy and its status.
 * 
 * @author christina.strobel
 * 
 */
public interface BounceProxyStatusInformation {

    /**
     * Returns the identifier of a bounce proxy.
     * 
     * @return the id of the bounce proxy
     */
    public String getBounceProxyId();

    /**
     * Returns the current status of a proxy.
     * 
     * @return the current status of the proxy
     */
    public BounceProxyStatus getStatus();

    /**
     * Returns the freshness of this information, i.e. the timestamp when status
     * information about this bounce proxy was updated last.
     * 
     * @return the freshness of this information (Date)
     */
    public Date getFreshness();

    /**
     * Returns performance measures of this bounce proxy instance.
     * 
     * @return the performane measures information of this bounce proxy
     */
    public PerformanceMeasures getPerformanceMeasures();
}
