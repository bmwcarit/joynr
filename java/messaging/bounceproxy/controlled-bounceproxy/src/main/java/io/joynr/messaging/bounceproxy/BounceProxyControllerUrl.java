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

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import com.google.inject.Inject;
import com.google.inject.name.Named;

/**
 * Convenience class to retrieve URLs for different services of the Bounce Proxy
 * Controller.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyControllerUrl {

    private static final String URL_PATH_SEPARATOR = "/";
    private static final String LIFECYCLE_PATH = "lifecycle";

    private String baseUrl;

    private String bounceProxyId;

    /**
     * Creates a new Bounce Proxy Controller URL class from a base URL.
     * 
     * @param baseUrl
     */
    @Inject
    public BounceProxyControllerUrl(@Named(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_CONTROLLER_BASE_URL) final String baseUrl,
                                    @Named(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_ID) final String bounceProxyId) {
        if (!baseUrl.endsWith(URL_PATH_SEPARATOR)) {
            this.baseUrl = baseUrl + URL_PATH_SEPARATOR;
        } else {
            this.baseUrl = baseUrl;
        }

        this.bounceProxyId = bounceProxyId;
    }

    /**
     * Returns the URL including query parameters to report bounce proxy
     * startup.
     * 
     * @param controlledBounceProxyUrl
     * 
     * @return the url including encoded query parameters
     * @throws UnsupportedEncodingException
     *             if urls passed as query parameters could not be encoded
     *             correctly.
     */
    public String buildReportStartupUrl(ControlledBounceProxyUrl controlledBounceProxyUrl)
                                                                                          throws UnsupportedEncodingException {
        String url4cc = URLEncoder.encode(controlledBounceProxyUrl.getOwnUrlForClusterControllers(), "UTF-8");
        String url4bpc = URLEncoder.encode(controlledBounceProxyUrl.getOwnUrlForBounceProxyController(), "UTF-8");

        return baseUrl + //
                "?bpid=" + this.bounceProxyId + //
                "&url4cc=" + url4cc + //
                "&url4bpc=" + url4bpc;
    }

    /**
     * Returns the URL including path and query parameters to report bounce
     * proxy shutdown.
     * 
     * @return
     */
    public String buildReportShutdownUrl() {
        return this.baseUrl + this.bounceProxyId + "/" + LIFECYCLE_PATH + "?status=shutdown";
    }

}
