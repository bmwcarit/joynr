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

import static io.joynr.systemintegrationtest.jee.JoynrConfigurationProvider.CONTROLLER_DOMAIN;
import static io.joynr.systemintegrationtest.jee.JoynrConfigurationProvider.SIT_DOMAIN_PREFIX;

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
@ProviderDomain(value = CONTROLLER_DOMAIN)
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class ControllerBean implements SitControllerSync {

    private static final Logger logger = LoggerFactory.getLogger(ControllerBean.class);

    private ServiceLocator serviceLocator;

    @Inject
    public ControllerBean(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
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
    public String triggerTests() {
        logger.info("triggerTests called");
        StringBuffer result = new StringBuffer();
        for (String appendValue : new String[]{ ".jee", ".cpp", ".java", ".node" }) {
            callProducer(SIT_DOMAIN_PREFIX + appendValue, result);
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
