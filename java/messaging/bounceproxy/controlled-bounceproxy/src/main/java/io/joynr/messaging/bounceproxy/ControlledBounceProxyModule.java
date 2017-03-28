package io.joynr.messaging.bounceproxy;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import io.joynr.messaging.bounceproxy.info.BounceProxyInformationProvider;
import io.joynr.messaging.bounceproxy.monitoring.BounceProxyLifecycleMonitor;
import io.joynr.messaging.bounceproxy.monitoring.BounceProxyPerformanceMonitor;
import io.joynr.messaging.bounceproxy.monitoring.MonitoringServiceClient;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.service.MessagingService;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;

public class ControlledBounceProxyModule extends AbstractModule {

    /*
     * (non-Javadoc)
     * 
     * @see com.google.inject.AbstractModule#configure()
     */
    @Override
    protected void configure() {
        bind(MessagingService.class).to(MessagingServiceImpl.class);

        bind(BounceProxyLifecycleMonitor.class).to(MonitoringServiceClient.class);
        bind(BounceProxyInformation.class).toProvider(BounceProxyInformationProvider.class);
    }

    @Provides
    CloseableHttpClient getCloseableHttpClient() {
        // For now, we provide the httpclient in here, as the configuration is
        // different from messaging configuration (e.g. no proxy settings).
        // If a custom configuration is needed, a separate provider should be
        // created.
        return HttpClients.createDefault();
    }

    @Provides
    @Singleton
    ExecutorService getExecutorService() {
        return Executors.newSingleThreadExecutor();
    }

    @Provides
    @Singleton
    ScheduledExecutorService getScheduledExecutorService() {
        return Executors.newSingleThreadScheduledExecutor();
    }

    /**
     * TODO This is a temporary solution until bounce proxy is implemented and
     * actual performance measures can be monitored.
     * 
     * @return
     */
    @Provides
    BounceProxyPerformanceMonitor getBounceProxyPerformanceMonitor() {
        return new BounceProxyPerformanceMonitor() {

            @Override
            public Map<String, Integer> getAsKeyValuePairs() {
                HashMap<String, Integer> map = new HashMap<String, Integer>();
                map.put("activeLongPolls", 0);
                map.put("assignedChannels", 0);
                return map;
            }
        };
    }
}