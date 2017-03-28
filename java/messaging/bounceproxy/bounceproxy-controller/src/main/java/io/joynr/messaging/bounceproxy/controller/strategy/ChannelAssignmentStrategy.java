package io.joynr.messaging.bounceproxy.controller.strategy;

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

import io.joynr.exceptions.JoynrChannelNotAssignableException;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

/**
 * Interface for strategies to assign channels to bounce proxy instances.
 * 
 * @author christina.strobel
 * 
 */
public interface ChannelAssignmentStrategy {

    /**
     * Calculates a bounce proxy instance that is responsible to handle
     * messaging for the channel ID.<br>
     * 
     * Depending on the strategy, this will calculate the best bounce proxy
     * instance to take over messaging for the channel. It will not change the
     * state of any databases containing channel assignments or bounce proxy
     * records.
     * 
     * @param ccid
     *            the channel ID
     * @return information about the responsible bounce proxy. Should never
     *         return <code>null</code>.
     * @throws JoynrChannelNotAssignableException
     *             if no bounce proxy instance could be found to handle the
     *             channel
     */
    ControlledBounceProxyInformation calculateBounceProxy(String ccid) throws JoynrChannelNotAssignableException;

}
