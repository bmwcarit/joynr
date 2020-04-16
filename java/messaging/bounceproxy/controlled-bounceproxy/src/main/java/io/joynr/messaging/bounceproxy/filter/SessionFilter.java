/*
 * #%L
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
package io.joynr.messaging.bounceproxy.filter;

import java.io.IOException;
import java.net.URI;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.SessionTrackingMode;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpServletResponseWrapper;
import javax.servlet.http.HttpSession;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Filter that checks that every request has a valid session for the request.<br>
 * For the chosen URLs it's not possible to define separate filters for channel
 * setup and messaging requests (URL patterns can't be defined). Thus the filter
 * decides to let the request pass or not depending on the URL pattern.
 * 
 * @author christina.strobel
 * 
 */
@Singleton
public class SessionFilter implements Filter {

    private static final String PROPERTY_SESSION_ID_NAME = "joynr.bounceproxy.session_id_name";
    private static final String PROPERTY_ROUTE_ID_NAME = "joynr.bounceproxy.route_id_name";

    private static final Logger logger = LoggerFactory.getLogger(SessionFilter.class);

    @Inject
    @Named(PROPERTY_SESSION_ID_NAME)
    private String sessionIdName;

    @Inject
    @Named(PROPERTY_ROUTE_ID_NAME)
    private String routeIdName;

    private String routeId;

    @Override
    public void init(FilterConfig filterConfig) throws ServletException {
        routeId = System.getProperty(routeIdName);
        logger.debug("Using routeId {}", routeId);
    }

    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException,
                                                                                              ServletException {

        // TODO check for session validity later on. Right now we just check if
        // there's a session and if not, we create a new one.

        HttpServletRequest httpRequest = (HttpServletRequest) request;

        if (!hasSession(httpRequest)) {
            HttpSession session = httpRequest.getSession(true);
            logger.info("Creating new session for request: session ID: {}", session.getId());
        } else {
            logger.debug("Request with existing session: {}", httpRequest.getSession().getId());
        }

        chain.doFilter(request, new UrlRewritingResponseWrapper((HttpServletResponse) response, httpRequest));
    }

    private boolean hasSession(HttpServletRequest httpRequest) {

        HttpSession session = httpRequest.getSession(false);

        if (session != null) {
            return false;
        } else {
            return true;
        }
    }

    @Override
    public void destroy() {
    }

    /**
     * Response wrapper that encodes URLs that can be returned by joynr
     * services. <br>
     * This wrapper is used to have more control over URL rewriting for session
     * tracking. In other cases some properties might prevent URL rewriting,
     * e.g. if the request URL and the returned location do not have the same
     * host or protocol.<br>
     * This encodes only joynr locations returned by joynr services, i.e. the
     * encoding of locations that have query parameters, are relative or are
     * internal references aren't rewritten.
     */
    class UrlRewritingResponseWrapper extends HttpServletResponseWrapper {

        private final HttpServletRequest request;

        public UrlRewritingResponseWrapper(HttpServletResponse response, HttpServletRequest httpRequest) {
            super(response);
            this.request = httpRequest;
        }

        @Override
        public String encodeURL(String url) {
            return encodeJoynrLocation(url);
        }

        @Override
        public String encodeRedirectURL(String url) {
            return encodeJoynrLocation(url);
        }

        private String encodeJoynrLocation(String url) {

            if (url == null) {
                return null;
            }
            // we expect only absolute path resource locations to be encoded,
            // thus no query parameters, no internal references and only
            // absolute URLs
            URI uri = URI.create(url);
            if (uri.getQuery() != null || !uri.isAbsolute()) {
                return url;
            }

            // check if there's a session
            HttpSession session = request.getSession(false);
            if (session == null) {
                // no URL encoding needed
                return url;
            }

            // check if sessions are tracked via URLs
            if (!request.getServletContext().getEffectiveSessionTrackingModes().contains(SessionTrackingMode.URL)) {
                return url;
            }

            // encode the URL
            String sessionId = session.getId();

            // append jsessionid
            url = url + ";" + sessionIdName + "=" + sessionId;

            // append route id
            if (routeId != null) {
                url = url + "." + routeId;
            }

            return url;
        }
    }
}
