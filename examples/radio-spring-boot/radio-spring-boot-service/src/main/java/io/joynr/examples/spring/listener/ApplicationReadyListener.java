/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.examples.spring.listener;

import io.joynr.examples.spring.service.ConsumerService;
import io.joynr.examples.spring.service.ProviderService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.context.event.ApplicationReadyEvent;
import org.springframework.context.event.EventListener;
import org.springframework.stereotype.Component;

@Component
public class ApplicationReadyListener {

    private static final Logger logger = LoggerFactory.getLogger(ApplicationReadyListener.class);

    @Autowired
    private ProviderService providerService;
    @Autowired
    private ConsumerService consumerService;

    @EventListener
    public void applicationIsReady(final ApplicationReadyEvent event) {
        logger.info("Application ready event received.");
        logger.info("Initializing Provider registration.");
        providerService.getProvider();
        logger.info("Initializing Consumer registration.");
        consumerService.getConsumer();
        logger.info("Provider and Consumer registration finished.");
    }
}
