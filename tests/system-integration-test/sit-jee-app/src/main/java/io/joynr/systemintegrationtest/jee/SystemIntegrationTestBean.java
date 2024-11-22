/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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

import jakarta.ejb.Stateless;
import jakarta.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import joynr.test.SystemIntegrationTestSync;

@Stateless
@ServiceProvider(serviceInterface = SystemIntegrationTestSync.class)
public class SystemIntegrationTestBean implements SystemIntegrationTestSync {

    private static final Logger logger = LoggerFactory.getLogger(SystemIntegrationTestBean.class);

    private JoynrCallingPrincipal joynrCallerPrincipal;

    @Inject
    public SystemIntegrationTestBean(JoynrCallingPrincipal joynrCallerPrincipal) {
        this.joynrCallerPrincipal = joynrCallerPrincipal;
    }

    @Override
    public Integer add(Integer addendA, Integer addendB) {
        logger.info("SIT INFO Being called by: " + joynrCallerPrincipal.getUsername());
        return addendA + addendB;
    }

}
