package io.joynr.messaging;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import javax.inject.Named;

import org.apache.http.impl.client.CloseableHttpClient;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.AbstractModule;
import com.google.inject.Provides;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import com.google.inject.Singleton;

import io.joynr.messaging.http.operation.HttpClientProvider;
import joynr.infrastructure.ChannelUrlDirectoryProxy;

public class MessagingTestModule extends AbstractModule {

    @Mock
    private ChannelUrlDirectoryProxy mockChannelUrlClient;

    public MessagingTestModule() {
        MockitoAnnotations.initMocks(this);

    }

    @Override
    protected void configure() {
        bind(MessagingSettings.class).to(ConfigurableMessagingSettings.class);
        bind(MessageSender.class).to(HttpMessageSenderImpl.class);
        // don't override like this. Override via properties passed to createJoynInjector
        // bind(String.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_BOUNCE_PROXY_URL)).toInstance(bounceProxyUrl);

        // binding global channelurl interface to local as well, to prevent channelurl comm.
        // this leads to endless loops
        // bind(ChannelUrlDirectoryClient.class).to(LocalChannelUrlDirectoryClientImpl.class);
        bind(ChannelUrlDirectoryProxy.class).toInstance(mockChannelUrlClient);
        bind(CloseableHttpClient.class).toProvider(HttpClientProvider.class).in(Singleton.class);

    }

    @Provides
    @Named(MessageScheduler.SCHEDULEDTHREADPOOL)
    ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor(@Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maximumParallelSends) {
        ThreadFactory schedulerNamedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.MessageScheduler-scheduler-%d")
                                                                              .build();
        ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(maximumParallelSends,
                                                                                schedulerNamedThreadFactory);
        scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
        scheduler.allowCoreThreadTimeOut(true);
        return scheduler;
    }
}
