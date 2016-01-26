package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;

import java.io.IOException;

import joynr.JoynrMessage;
import joynr.system.RoutingProvider;
import joynr.system.RoutingTypes.Address;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

public interface MessageRouter extends RoutingProvider {
    static final String SCHEDULEDTHREADPOOL = "io.joynr.messaging.scheduledthreadpool";

    public void route(JoynrMessage message) throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                           JsonGenerationException, JsonMappingException, IOException;

    public void addNextHop(String participantId, Address address);

    public void shutdown();

}
