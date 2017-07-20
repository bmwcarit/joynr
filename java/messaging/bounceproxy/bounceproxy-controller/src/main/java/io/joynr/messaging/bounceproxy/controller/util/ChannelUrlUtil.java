package io.joynr.messaging.bounceproxy.controller.util;

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

import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.net.URI;

/**
 * Utility class for tweaking of channel URLs.
 * 
 * @author christina.strobel
 * 
 */
public class ChannelUrlUtil {

    /**
     * Creates the channel location that is returned to the cluster controller.
     * This includes tweaking of the URL for example to contain a session ID.
     * 
     * @param bpInfo
     *            information of the bounce proxy
     * @param ccid
     *            channel identifier
     * @param location
     *            the channel URL as returned by the bounce proxy instance that
     *            created the channel
     * @return channel url that can be used by cluster controllers to do
     *         messaging on the created channel
     */
    public static URI createChannelLocation(ControlledBounceProxyInformation bpInfo, String ccid, URI location) {

        return URI.create(location.toString() + ";jsessionid=." + bpInfo.getInstanceId());
    }

}
