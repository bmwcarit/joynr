/*-
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
package io.joynr.examples.customheaders;

import javax.ejb.Stateless;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.JoynrJeeMessageMetaInfo;
import io.joynr.jeeintegration.api.ServiceProvider;
import joynr.examples.customheaders.HeaderPingSync;

@Stateless
@ServiceProvider(serviceInterface = HeaderPingSync.class)
public class HeaderPingServiceBean implements HeaderPingSync {

    private static final Logger logger = LoggerFactory.getLogger(HeaderPingServiceBean.class);

    @Inject
    private JoynrJeeMessageMetaInfo jeeMessageMetaInfo;

    @Override
    public String ping() {
        String customHeaderValue = (String) jeeMessageMetaInfo.getMessageContext().get("application-custom-header");
        logger.info("Got application custom header {}", customHeaderValue);

        String processorCustomHeaderValue = (String) jeeMessageMetaInfo.getMessageContext()
                                                                       .get("processor-custom-header");
        logger.info("Got processor custom header {}", processorCustomHeaderValue);
        return String.format("msgQos[%s] / proc[%s]", customHeaderValue, processorCustomHeaderValue);
    }
}
