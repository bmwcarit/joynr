package io.joynr.messaging.bounceproxy;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import joynr.JoynrMessage;
import io.joynr.messaging.service.MessagingService;

/**
 * Implementation of messaging service for controlled bounce proxies.
 * 
 * @author christina.strobel
 *
 */
public class MessagingServiceImpl implements MessagingService {

    /* (non-Javadoc)
     * @see io.joynr.messaging.service.MessagingService#hasMessageReceiver(java.lang.String)
     */
    @Override
    public boolean hasMessageReceiver(String ccid) {
        // TODO Auto-generated method stub
        return false;
    }

    /* (non-Javadoc)
     * @see io.joynr.messaging.service.MessagingService#passMessageToReceiver(java.lang.String, joynr.JoynrMessage)
     */
    @Override
    public void passMessageToReceiver(String ccid, JoynrMessage message) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see io.joynr.messaging.service.MessagingService#isAssignedForChannel(java.lang.String)
     */
    @Override
    public boolean isAssignedForChannel(String ccid) {
        // TODO Auto-generated method stub
        return false;
    }

    /* (non-Javadoc)
     * @see io.joynr.messaging.service.MessagingService#hasChannelAssignmentMoved(java.lang.String)
     */
    @Override
    public boolean hasChannelAssignmentMoved(String ccid) {
        // TODO Auto-generated method stub
        return false;
    }

}
