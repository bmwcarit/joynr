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
package io.joynr.jeeintegration.context;

import javax.enterprise.event.Observes;
import javax.enterprise.inject.spi.AfterBeanDiscovery;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.inject.spi.BeforeBeanDiscovery;
import javax.enterprise.inject.spi.Extension;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.JoynrJeeMessageScoped;

/**
 * Portable CDI extension which registers the {@link JoynrJeeMessageContext} for the {@link JoynrJeeMessageScoped joynr
 * JEE message scope}.
 */
public class RegisterJoynrJeeMessageContextExtension implements Extension {

    private static final Logger logger = LoggerFactory.getLogger(RegisterJoynrJeeMessageContextExtension.class);

    public void afterBeanDiscovery(@Observes AfterBeanDiscovery event, BeanManager beanManager) {
        JoynrJeeMessageContext context = JoynrJeeMessageContext.getInstance();
        logger.info("Created new JoynrJeeMessageContext {} and adding to event {}", context, event);
        event.addContext(context);
    }

    public void beforeBeanDiscovery(@Observes BeforeBeanDiscovery event) {
        logger.info("Adding JoynrJeeMessageScoped scope to event {}", event);
        event.addScope(JoynrJeeMessageScoped.class, true, false);
    }

}
