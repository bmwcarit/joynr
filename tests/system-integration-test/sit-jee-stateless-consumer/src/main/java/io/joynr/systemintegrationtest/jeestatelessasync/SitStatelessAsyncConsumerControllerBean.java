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

import static io.joynr.systemintegrationtest.jeestatelessasync.JoynrConfigurationProvider.CONTROLLER_DOMAIN;
import static io.joynr.systemintegrationtest.jeestatelessasync.JoynrConfigurationProvider.SIT_DOMAIN_PREFIX;
import static io.joynr.systemintegrationtest.jeestatelessasync.SitStatelessAsyncCallback.USE_CASE;

import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
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
@ProviderDomain(value = CONTROLLER_DOMAIN)
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class SitStatelessAsyncConsumerControllerBean implements SitControllerSync {

    private static final Logger logger = LoggerFactory.getLogger(SitStatelessAsyncConsumerControllerBean.class);

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
    public String triggerTests(String domains, Boolean expectFailure) {
        logger.info("triggerTests called");
        StringBuffer result = new StringBuffer();
        String configuredDomain = System.getenv("SIT_DOMAIN");
        //callProducer(SIT_DOMAIN_PREFIX + ".node", result);
        callProducer(configuredDomain + "_" + SIT_DOMAIN_PREFIX + ".jee", result);
        return result.toString();
    }

    @Override
    public String waitForStatelessResult(Integer timeoutMs) {
        logger.info("waitForStatelessResult called");
        try {
            return Base64.getMimeEncoder()
                         .encodeToString(resultQueue.getResult(timeoutMs).getBytes(StandardCharsets.UTF_8));
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

            String[] gbids = new String[]{ "joynrdefaultgbid", "othergbid" };

            SystemIntegrationTestStatelessAsync proxy = serviceLocator.builder(SystemIntegrationTestStatelessAsync.class,
                                                                               domain)
                                                                      .withUseCase(USE_CASE)
                                                                      .withDiscoveryQos(discoveryQos)
                                                                      .withMessagingQos(new MessagingQos())
                                                                      .withGbids(gbids)
                                                                      .build();

            final int numberOfWorkers = 2;
            final int numberOfRequests = 200;
            final int numberOfRequestsPerWorker = numberOfRequests / numberOfWorkers;
            CountDownLatch countDownLatch = new CountDownLatch(numberOfRequests);
            ExecutorService pool = Executors.newFixedThreadPool(numberOfWorkers);
            for (int i = 0; i < numberOfWorkers; i++) {
                final int offset = i * numberOfRequestsPerWorker;
                Runnable task = () -> {
                    for (int j = 0; j < numberOfRequestsPerWorker; j++) {
                        final int requestNumber = offset + j + 1;
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
                };
                pool.submit(task);
            }
            pool.shutdown();
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
