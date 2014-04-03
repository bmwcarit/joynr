package io.joynr.messaging.bounceproxy.controller.exception;

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

import io.joynr.exceptions.JoynrException;

/**
 * Thrown if an assignment of a channel to a bounce proxy instance failed.
 * 
 * @author christina.strobel
 *
 */
public class JoynrChannelNotAssignableException extends JoynrException {

    private static final long serialVersionUID = 212450268038844695L;

    /**
     * The channel that could not be assigned to a bounce proxy.
     */
    private String ccid;

    /**
     * @param message
     * @param ccid the channel ID that could not be assigned.
     */
    public JoynrChannelNotAssignableException(String message, String ccid) {
        super(message + " for channel '" + ccid + "'");
        this.ccid = ccid;
    }

    public String getChannelId() {
        return ccid;
    }
}
