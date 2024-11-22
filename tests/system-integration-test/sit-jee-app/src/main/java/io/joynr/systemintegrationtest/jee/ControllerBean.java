/*
 * #%L
 * %%
 * Copyright (C) 2024 BMW Car IT GmbH
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

import static io.joynr.systemintegrationtest.jee.JoynrConfigurationProvider.SIT_DOMAIN_PREFIX;

import java.nio.charset.StandardCharsets;
import java.util.Base64;

import jakarta.ejb.ConcurrencyManagement;
import jakarta.ejb.ConcurrencyManagementType;
import jakarta.ejb.Stateless;
import jakarta.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.jeeintegration.api.ServiceProvider;
import joynr.test.SitControllerSync;
import joynr.test.SystemIntegrationTestSync;

@Stateless
@ServiceProvider(serviceInterface = SitControllerSync.class)
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class ControllerBean implements SitControllerSync {

    private static final Logger logger = LoggerFactory.getLogger(ControllerBean.class);

    private ServiceLocator serviceLocator;
    private String configuredDomain;

    @Inject
    public ControllerBean(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
        configuredDomain = System.getenv("SIT_DOMAIN");
    }

    @Override
    public String ping() {
        logger.info("ping called");
        return "OK";
    }

    @Override
    public String waitForStatelessResult(Integer timeoutMs) {
        String errorMsg = "SIT RESULT error: waitForStatelessResult NOT IMPLEMENTED";
        logger.error(errorMsg);
        return errorMsg;
    }

    @Override
    public String triggerTests(String domains, Boolean expectFailure) {
        logger.info("triggerTests called \n");
        StringBuffer result = new StringBuffer();
        if (expectFailure) {
            callProducerWithExpectedFailure("failure_" + SIT_DOMAIN_PREFIX, result);
        } else {
            logger.info("testDomains: " + domains + " \n");
            String[] testDomains = domains.split("\\|");
            for (String domain : testDomains) {
                logger.info("received domain " + domain + "\n");
            }
            for (String appendValue : new String[]{ ".jee", ".java", ".cpp", ".node" }) {
                for (String testDomain : testDomains) {
                    callProducer(testDomain + "_" + SIT_DOMAIN_PREFIX + appendValue, result);
                }
            }
        }
        return Base64.getMimeEncoder().encodeToString(result.toString().getBytes(StandardCharsets.UTF_8));
    }

    private void callProducer(String domain, StringBuffer result) {
        try {
            logger.info("callProducer with domain " + domain + " called \n");
            String[] configuredGbids = System.getenv("SIT_GBIDS").trim().split(",");
            DiscoveryQos discoveryQos = new DiscoveryQos();
            discoveryQos.setDiscoveryTimeoutMs(90000); // 90 Seconds
            SystemIntegrationTestSync proxy = serviceLocator.builder(SystemIntegrationTestSync.class, domain)
                                                            .withDiscoveryQos(discoveryQos)
                                                            .withGbids(configuredGbids)
                                                            .build();
            Integer additionResult = proxy.add(1, 1);
            if (additionResult != 2) {
                throw new IllegalArgumentException("1 + 1 should be 2, got: " + additionResult);
            }
            result.append("SIT RESULT success: JEE consumer ").append(configuredDomain).append(" -> ").append(domain);
        } catch (Exception e) {
            logger.error("Exception in callProducer: " + e);
            result.append("SIT RESULT error: JEE consumer ")
                  .append(configuredDomain)
                  .append(" -> ")
                  .append(domain)
                  .append("\nException: ")
                  .append(e.toString());
            Util.addStacktraceToResultString(e, result);
        }
        result.append("\n");
    }

    private void callProducerWithExpectedFailure(String domain, StringBuffer result) {
        try {
            logger.info("callProducerWithExpectedFailure called \n");
            DiscoveryQos discoveryQos = new DiscoveryQos();
            discoveryQos.setDiscoveryTimeoutMs(10000); // 10 Seconds
            SystemIntegrationTestSync proxy = serviceLocator.builder(SystemIntegrationTestSync.class, domain)
                                                            .withDiscoveryQos(discoveryQos)
                                                            .withGbids(new String[]{ "invalid" })
                                                            .build();
            Integer additionResult = proxy.add(1, 1);
            if (additionResult != 2) {
                throw new IllegalArgumentException("1 + 1 should be 2, got: " + additionResult);
            }
            result.append("SIT RESULT error: JEE consumer ").append(configuredDomain).append(" -> ").append(domain);
        } catch (JoynrRuntimeException e) {
            result.append("SIT RESULT success: JEE consumer ").append(configuredDomain).append(" -> ").append(domain);
        } catch (Exception e) {
            result.append("SIT RESULT error: JEE consumer ")
                  .append(configuredDomain)
                  .append(" -> ")
                  .append(domain)
                  .append("\nException: ")
                  .append(e.toString());
            Util.addStacktraceToResultString(e, result);
        }
        result.append("\n");
    }
}
