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
package io.joynr.systemintegrationtest.jee;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import javax.inject.Inject;

import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.messaging.MessagingQos;
import joynr.test.SystemIntegrationTestSync;

/**
 * Exposes a REST endpoint with which the consumer side joynr system integration tests can be triggered.
 */
@Path("/consumer")
@Produces(MediaType.APPLICATION_JSON)
public class ConsumerRestEndpoint {

    private static final Logger logger = LoggerFactory.getLogger(ConsumerRestEndpoint.class);

    private ServiceLocator serviceLocator;

    private DomainPrefixProvider domainPrefixProvider;

    @Inject
    public ConsumerRestEndpoint(ServiceLocator serviceLocator, DomainPrefixProvider domainPrefixProvider) {
        this.serviceLocator = serviceLocator;
        this.domainPrefixProvider = domainPrefixProvider;
    }

    @GET
    @Path("/ping")
    public String ping() {
        return "OK";
    }

    @GET
    public String triggerTests() {
        StringBuffer result = new StringBuffer();
        for (String domainPrefix : domainPrefixProvider.getDomainPrefixes()) {
            for (String appendValue : new String[]{ ".jee", ".cpp", ".java", ".node" }) {
                callProducer(domainPrefix + appendValue, result);
            }
        }
        return result.toString();
    }

    private void callProducer(String domain, StringBuffer result) {
        try {
            DiscoveryQos discoveryQos = new DiscoveryQos();
            discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Minutes
            SystemIntegrationTestSync proxy = serviceLocator.get(SystemIntegrationTestSync.class,
                                                                 domain,
                                                                 new MessagingQos(),
                                                                 discoveryQos);
            Integer additionResult = proxy.add(1, 1);
            if (additionResult != 2) {
                throw new IllegalArgumentException("1 + 1 should be 2, got: " + additionResult);
            }
            result.append("SIT RESULT success: JEE consumer -> ").append(domain);
        } catch (Exception e) {
            result.append("SIT RESULT error: JEE consumer -> ")
                  .append(domain)
                  .append("\nException: ")
                  .append(e.toString());
            addStacktrace(e, result);
        }
        result.append("\n");
    }

    private void addStacktrace(Exception theException, StringBuffer result) {
        try (StringWriter writer = new StringWriter(); PrintWriter printWriter = new PrintWriter(writer)) {
            theException.printStackTrace(printWriter);
            result.append(writer.toString());
        } catch (IOException e) {
            logger.error("Unable to add exception stacktrace to result string.", e);
        }
    }
}
