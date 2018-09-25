/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.systemintegrationtest.jeestatelessasync;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import javax.ejb.ConcurrencyManagement;
import javax.ejb.ConcurrencyManagementType;
import javax.ejb.Stateless;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.jeeintegration.api.ProviderDomain;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.messaging.MessagingQos;
import io.joynr.systemintegrationtest.jee.Util;
import joynr.exceptions.ProviderRuntimeException;
import joynr.test.SitControllerSync;
import joynr.test.SystemIntegrationTestStatelessAsync;

@Stateless
@ServiceProvider(serviceInterface = SitControllerSync.class)
@ProviderDomain(value = SitStatelessAsyncConsumerControllerBean.CONTROLLER_DOMAIN)
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class SitStatelessAsyncConsumerControllerBean implements SitControllerSync {
    private static final String SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String CONTROLLER_DOMAIN_PREFIX = SIT_DOMAIN_PREFIX + ".controller";
    protected static final String CONTROLLER_DOMAIN = CONTROLLER_DOMAIN_PREFIX + ".jee-stateless-consumer";

    private static final Logger logger = LoggerFactory.getLogger(SitStatelessAsyncConsumerControllerBean.class);

    private static final String USE_CASE = "sit-jee-stateless-async";

    private ServiceLocator serviceLocator;
    private StatelessResultQueue resultQueue;

    @Inject
    public SitStatelessAsyncConsumerControllerBean(ServiceLocator serviceLocator, StatelessResultQueue resultQueue) {
        this.serviceLocator = serviceLocator;
        this.resultQueue = resultQueue;
    }

    @Override
    public String ping() {
        logger.info("ping called");
        return "OK";
    }

    @Override
    public String triggerTests() {
        logger.info("triggerTests called");
        StringBuffer result = new StringBuffer();
        callProducer(SIT_DOMAIN_PREFIX + ".node", result);
        return result.toString();
    }

    @Override
    public String waitForResult(Integer timeoutMs) {
        logger.info("waitForResult called");
        try {
            return resultQueue.getResult(timeoutMs);
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: stateless async JEE consumer -> " + e.toString();
            logger.error(errorMsg);
            return errorMsg;
        }
    }

    private void callProducer(String domain, StringBuffer result) {
        try {
            DiscoveryQos discoveryQos = new DiscoveryQos();
            discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Minutes

            SystemIntegrationTestStatelessAsync proxy = serviceLocator.builder(SystemIntegrationTestStatelessAsync.class,
                                                                               domain)
                                                                      .withUseCase(USE_CASE)
                                                                      .withDiscoveryQos(discoveryQos)
                                                                      .withMessagingQos(new MessagingQos())
                                                                      .build();

            final int iterations = 10;
            CountDownLatch countDownLatch = new CountDownLatch(iterations);
            for (int i = 0; i < iterations; i++) {
                final int requestNumber = i + 1;
                proxy.add(requestNumber, 1, messageId -> {
                    result.append("\nSIT REQUEST: ")
                          .append(domain)
                          .append(": request #")
                          .append(requestNumber)
                          .append(": ")
                          .append(messageId);
                    countDownLatch.countDown();
                });
            }
            if (!countDownLatch.await(60, TimeUnit.SECONDS)) {
                String errorMsg = "SIT RESULT error: stateless async JEE consumer failed to send request";
                logger.error(errorMsg);
                result.append(errorMsg);
            }
        } catch (Exception e) {
            StringBuffer error = new StringBuffer();
            error.append("SIT RESULT error: stateless async JEE consumer -> ")
                 .append(domain)
                 .append("\nException: ")
                 .append(e.toString());
            Util.addStacktraceToResultString(e, error);
            logger.error(error.toString());
            throw new ProviderRuntimeException(error.toString());
        }
        result.append("\n");
    }

}
