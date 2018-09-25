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
package io.joynr.systemintegrationtest.jee;

import java.util.Base64;

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
import joynr.test.SitControllerSync;
import joynr.test.SystemIntegrationTestSync;

@Stateless
@ServiceProvider(serviceInterface = SitControllerSync.class)
@ProviderDomain(value = ControllerBean.CONTROLLER_DOMAIN)
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class ControllerBean implements SitControllerSync {
    private static final String SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String CONTROLLER_DOMAIN_PREFIX = SIT_DOMAIN_PREFIX + ".controller";
    protected static final String CONTROLLER_DOMAIN = CONTROLLER_DOMAIN_PREFIX + ".jee-app";

    private static final Logger logger = LoggerFactory.getLogger(ControllerBean.class);

    private ServiceLocator serviceLocator;

    private DomainPrefixProvider domainPrefixProvider;

    @Inject
    public ControllerBean(ServiceLocator serviceLocator, DomainPrefixProvider domainPrefixProvider) {
        this.serviceLocator = serviceLocator;
        this.domainPrefixProvider = domainPrefixProvider;
    }

    @Override
    public String ping() {
        logger.info("ping called");
        return "OK";
    }

    @Override
    public String waitForResult(Integer timeoutMs) {
        String errorMsg = "SIT RESULT error: waitForResult NOT IMPLEMENTED";
        logger.error(errorMsg);
        return errorMsg;
    }

    @Override
    public String triggerTests() {
        logger.info("triggerTests called");
        StringBuffer result = new StringBuffer();
        for (String domainPrefix : domainPrefixProvider.getDomainPrefixes()) {
            for (String appendValue : new String[]{ ".jee", ".cpp", ".java", ".node" }) {
                callProducer(domainPrefix + appendValue, result);
            }
        }
        return new String(Base64.getEncoder().encode(result.toString().getBytes()));
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
            Util.addStacktraceToResultString(e, result);
        }
        result.append("\n");
    }

}
