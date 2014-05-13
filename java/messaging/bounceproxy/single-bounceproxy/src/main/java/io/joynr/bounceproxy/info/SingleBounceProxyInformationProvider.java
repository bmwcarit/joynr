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

public class SingleBounceProxyInformationProvider implements Provider<BounceProxyInformation> {

    private static final Logger log = LoggerFactory.getLogger(SingleBounceProxyInformationProvider.class);

    private final BounceProxyInformation bpInfo;

    @Inject
    public SingleBounceProxyInformationProvider(HttpServletRequest request) {
        String bounceProxyServerName = request.getServerName();

        String scheme = request.getScheme();
        int port = request.getServerPort();
        String servletContext = request.getServletContext().getContextPath();
        URI bounceProxyUrl;
        try {
            bounceProxyUrl = new URI(scheme, null, bounceProxyServerName, port, servletContext, null, null);
        } catch (URISyntaxException e) {
            log.error("Error retrieving bounceproxy URL: error: {}. Trying simplified version.", e.getMessage());
            bounceProxyUrl = URI.create(String.format("%s://%s:%d/%s",
                                                      scheme,
                                                      bounceProxyServerName,
                                                      port,
                                                      servletContext));
        }
        log.info("Using bounceproxy URL {}", bounceProxyUrl);

        bpInfo = new BounceProxyInformation(bounceProxyServerName, URI.create(bounceProxyUrl.toString()));
    }

    @Override
    public BounceProxyInformation get() {
        return bpInfo;
    }

}
