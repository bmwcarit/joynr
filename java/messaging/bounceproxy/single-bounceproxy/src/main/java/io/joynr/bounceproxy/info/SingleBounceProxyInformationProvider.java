package io.joynr.bounceproxy.info;

/*
 * #%L
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
import java.net.URISyntaxException;

import javax.servlet.http.HttpServletRequest;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

/**
 * Builds a {@link BounceProxyInformation} object from either a hostpath set as
 * property or from the host path given in the servlet request.
 * 
 * @author christina.strobel
 * 
 */
public class SingleBounceProxyInformationProvider implements Provider<BounceProxyInformation> {

    private static final Logger log = LoggerFactory.getLogger(SingleBounceProxyInformationProvider.class);

    public static final String PROPERTY_SERVLET_HOST_PATH = "joynr.servlet.hostpath";

    private BounceProxyInformation bpInfo;

    private String hostPath = null;
    private URI hostPathFromRequest = null;

    @Inject(optional = true)
    public void setHostPath(@Named(PROPERTY_SERVLET_HOST_PATH) String hostPath) {
        this.hostPath = hostPath;

        if (this.hostPath != null) {
            log.info("Using bounceproxy URL {} from property {}", hostPath, PROPERTY_SERVLET_HOST_PATH);
            bpInfo = new BounceProxyInformation(hostPath, URI.create(hostPath));
        }
    }

    @Inject
    public SingleBounceProxyInformationProvider(HttpServletRequest request) {

        String bounceProxyServerName = request.getServerName();

        String scheme = request.getScheme();
        int port = request.getServerPort();
        String servletContext = request.getServletContext().getContextPath();
        try {
            hostPathFromRequest = new URI(scheme, null, bounceProxyServerName, port, servletContext, null, null);
        } catch (URISyntaxException e) {
            log.error("Error retrieving bounceproxy URL: error: {}. Trying simplified version.", e.getMessage());
            hostPathFromRequest = URI.create(String.format("%s://%s:%d/%s",
                                                           scheme,
                                                           bounceProxyServerName,
                                                           port,
                                                           servletContext));
        }
    }

    @Override
    public BounceProxyInformation get() {

        if (bpInfo == null) {
            log.info("No bounceproxy URL set from properties. Using bounceproxy URL {} created from request",
                     hostPathFromRequest);
            bpInfo = new BounceProxyInformation(hostPathFromRequest.getHost(), hostPathFromRequest);
        }

        return bpInfo;
    }

}
