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
package io.joynr.integration.setup;

import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import javax.servlet.RequestDispatcher;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.handler.ContextHandler;
import org.eclipse.jetty.server.session.SessionHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.util.Utilities;

/**
 * Request dispatcher for testing purposes.<br>
 * The dispatcher uses these rules to dispatch requests:
 * <ul>
 * <li>If the request is made without a session, the requests are dispatched
 * round robin.</li>
 * <li>If there's a session id in the request, the request is forwarded to a
 * certain instance:
 * <ul>
 * <li>If a server instance ID is encoded in the session (i.e. session format is
 * [sessionId].[jvmRouteId]), the request is routed to the server instance that
 * represents the jvmRouteId. This means that the request is forwarded to the
 * server instance that handled the request in which the session ID was created.
 * </li>
 * <li>Otherwise the request is routed to the server instance that returned the
 * first request associated with the session.</li>
 * </ul>
 * </ul>
 * 
 * @author christina.strobel
 *
 */
class TestRequestDispatcher extends ContextHandler {

    static final Logger logger = LoggerFactory.getLogger(TestRequestDispatcher.class);

    private final HashMap<String, String> sessionStore;
    private final Map<Integer, ClusterNode> serverInstances;

    private Iterator<ClusterNode> currentServerInstanceIndex;

    private final Map<String, Integer> contextForwardCounts = new HashMap<String, Integer>();

    public TestRequestDispatcher(HashMap<String, String> sessionStore, Map<Integer, ClusterNode> serverInstances) {
        setHandler(new SessionHandler());
        this.sessionStore = sessionStore;
        this.serverInstances = serverInstances;
        currentServerInstanceIndex = serverInstances.values().iterator();
    }

    @Override
    public void doHandle(String target,
                         Request baseRequest,
                         HttpServletRequest request,
                         HttpServletResponse response) throws IOException, ServletException {

        String sessionId = request.getRequestedSessionId();

        if (sessionId == null) {
            String contextPath = forwardRoundRobin(baseRequest, response);

            // check if a session has been created in this request
            String location = response.getHeader("Location");
            if (location != null && Utilities.isSessionEncodedInUrl(location, "jsessionid")) {
                String createdSessionId = Utilities.getSessionId(location, "jsessionid");

                ClusterNode sessionCreatingInstance = getSessionCreatingServerInstance(createdSessionId);

                if (sessionCreatingInstance != null) {
                    sessionStore.put(createdSessionId, sessionCreatingInstance.getContextPath());
                    logger.debug("Request created a session encoded url that was created at server instance {}. Stored session ID {} for that instance",
                                 sessionCreatingInstance.getContextPath(),
                                 createdSessionId);
                } else {
                    sessionStore.put(createdSessionId, contextPath);
                    logger.debug("Request created a session encoded url. Stored session ID {} for that context {}",
                                 createdSessionId,
                                 contextPath);
                }
            }

        } else {

            String targetPath = sessionStore.get(sessionId);

            if (targetPath == null) {
                targetPath = forwardRoundRobin(baseRequest, response);
                sessionStore.put(sessionId, targetPath);
                logger.debug("Created new target path {} for session {}", targetPath, sessionId);
            } else {
                logger.debug("Applying sticky session pattern for target path {} and session {}",
                             targetPath,
                             sessionId);
                forwardToUrl(targetPath, baseRequest, response);
            }

        }

    }

    @Override
    protected void doStart() throws Exception {
        super.doStart();

        contextForwardCounts.clear();
    }

    @Override
    protected void doStop() throws Exception {
        super.doStop();

        logger.info("Forwarding statistics for dispatcher context {}", getContextPath());
        for (String contextPath : contextForwardCounts.keySet()) {
            int count = contextForwardCounts.get(contextPath);
            logger.info("Forwarded {} requests to {}", count, contextPath);
        }
    }

    private ClusterNode getSessionCreatingServerInstance(String sessionId) {

        int routeIdIndex = sessionId.lastIndexOf('.');
        if (routeIdIndex > 0) {
            String routeId = sessionId.substring(routeIdIndex + 1);

            ClusterNode serverInstance = serverInstances.get(Integer.parseInt(routeId));
            return serverInstance;
        }
        return null;
    }

    private String forwardRoundRobin(Request baseRequest, HttpServletResponse response) throws ServletException,
                                                                                        IOException {

        String contextPath = getNextServerInstanceContext();
        forwardToUrl(contextPath, baseRequest, response);

        return contextPath;
    }

    private String getNextServerInstanceContext() {

        if (!currentServerInstanceIndex.hasNext()) {
            currentServerInstanceIndex = serverInstances.values().iterator();
        }

        ClusterNode nextNode = currentServerInstanceIndex.next();
        return nextNode.getContextPath();
    }

    private void forwardToUrl(final String targetContextPath,
                              Request baseRequest,
                              HttpServletResponse response) throws ServletException, IOException {

        logger.info("Forward request {} to context {}", baseRequest.toString(), targetContextPath);

        synchronized (contextForwardCounts) {
            int currentContextForwardCount = 0;
            if (contextForwardCounts.containsKey(targetContextPath)) {
                currentContextForwardCount = contextForwardCounts.get(targetContextPath);
            }
            contextForwardCounts.put(targetContextPath, currentContextForwardCount + 1);
        }

        ServletContext currentContext = baseRequest.getServletContext();
        String currentContextPath = currentContext.getContextPath();

        String forwardContextPath = currentContextPath.replace(ClusteredBounceProxyWithDispatcher.CONTEXT_DISPATCHER,
                                                               targetContextPath);

        ServletContext forwardContext = currentContext.getContext(forwardContextPath);
        String forwardPath = baseRequest.getPathInfo();
        RequestDispatcher requestDispatcher = forwardContext.getRequestDispatcher(forwardPath);

        requestDispatcher.forward(baseRequest, response);
    }
}
