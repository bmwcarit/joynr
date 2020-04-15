/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.jeeintegration;

import javax.annotation.PostConstruct;
import javax.ejb.Singleton;
import javax.ejb.Startup;

import io.joynr.statusmetrics.JoynrStatusMetrics;
import io.joynr.statusmetrics.JoynrStatusMetricsAggregator;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Singleton
@Startup
public class JeeJoynrStatusMetricsAggregator extends JoynrStatusMetricsAggregator implements JoynrStatusMetrics {

    private static final Logger LOG = LoggerFactory.getLogger(JoynrIntegrationBean.class);

    @PostConstruct
    public void notifyPostConstruct() {
        LOG.debug("JeeJoynrStatusMetricsAggregator constructed.");
    }
}
