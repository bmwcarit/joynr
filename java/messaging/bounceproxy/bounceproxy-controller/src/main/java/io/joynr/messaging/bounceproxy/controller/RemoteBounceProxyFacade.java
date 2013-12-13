package io.joynr.messaging.bounceproxy.controller;

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

import io.joynr.messaging.info.BounceProxyInformation;

import java.net.URI;

/**
 * Facade for a remote bounce proxy. This component encapsulates communication
 * with services of a bounce proxy.
 * 
 * @author christina.strobel
 * 
 */
public class RemoteBounceProxyFacade {

    public RemoteBounceProxyFacade() {
    }

    /**
     * Creates a channel on the remote bounce proxy.
     * @param bpInfo 
     * 
     * @param ccid
     * @param trackingId
     * @return
     */
    public URI createChannel(BounceProxyInformation bpInfo, String ccid, String trackingId) {

        // TODO to http post

        // TODO if there's no URI in the response, throw an exception instead of returning null
        return URI.create(bpInfo.getLocation() + "/channels/" + ccid);
    }

}
