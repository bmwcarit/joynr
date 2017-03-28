package io.joynr.messaging.bounceproxy;

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

import com.google.inject.Inject;
import com.google.inject.name.Named;

/**
 * Convenience class to build URLs for a controlled bounce proxy.
 * 
 * @author christina.strobel
 * 
 */
public class ControlledBounceProxyUrl {

    private static final String URL_PATH_SEPARATOR = "/";

    private String baseUrlForBpc;
    private String baseUrlForCc;

    @Inject
    public ControlledBounceProxyUrl(@Named(BounceProxyPropertyKeys.PROPERTY_BOUNCEPROXY_URL_FOR_CC) String baseUrlForCc,
                                    @Named(BounceProxyPropertyKeys.PROPERTY_BOUNCEPROXY_URL_FOR_BPC) String baseUrlForBpc) {

        if (!baseUrlForCc.endsWith(URL_PATH_SEPARATOR)) {
            this.baseUrlForCc = baseUrlForCc + URL_PATH_SEPARATOR;
        } else {
            this.baseUrlForCc = baseUrlForCc;
        }

        if (!baseUrlForBpc.endsWith(URL_PATH_SEPARATOR)) {
            this.baseUrlForBpc = baseUrlForBpc + URL_PATH_SEPARATOR;
        } else {
            this.baseUrlForBpc = baseUrlForBpc;
        }
    }

    public String getOwnUrlForClusterControllers() {
        return this.baseUrlForCc;
    }

    public String getOwnUrlForBounceProxyController() {
        return this.baseUrlForBpc;
    }

}
