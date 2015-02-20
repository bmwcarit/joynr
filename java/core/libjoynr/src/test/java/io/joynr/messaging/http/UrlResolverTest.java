package io.joynr.messaging.http;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingTestModule;
import io.joynr.runtime.JoynrBaseModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.util.Properties;
import java.util.UUID;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.util.Modules;

@RunWith(MockitoJUnitRunner.class)
public class UrlResolverTest {

    private String channelId = "UrlResolverTest_" + UUID.randomUUID().toString();

    @Mock
    private LocalChannelUrlDirectoryClient mockChannelUrlDir;

    private UrlResolver urlResolver;

    @Before
    public void setUp() throws Exception {

        Properties properties = new Properties();
        properties.put(MessagingPropertyKeys.CHANNELID, channelId);

        AbstractModule mockModule = new AbstractModule() {

            @Override
            protected void configure() {
                bind(LocalChannelUrlDirectoryClient.class).toInstance(mockChannelUrlDir);
            }

        };

        Injector injector = new JoynrInjectorFactory(new JoynrBaseModule(properties,
                                                                         Modules.override(new MessagingTestModule())
                                                                                .with(mockModule))).getInjector();
        urlResolver = injector.getInstance(UrlResolver.class);
    }

    @Test
    public void testMapDomainName() throws Exception {
        String uri = "http://myhost.com:80/x/y/z/index.html?name=xyz";
        String mapHost = urlResolver.mapHost(uri);
        assertEquals(mapHost, "http://localhost:9096/x/y/z/index.html?name=xyz");

        uri = "https://myhost.com:80/x/y/z/index.html?name=xyz#XYZ";
        mapHost = urlResolver.mapHost(uri);
        assertEquals(mapHost, "https://localhost:9096/x/y/z/index.html?name=xyz");

        uri = "https://myhost2.com:80/x/y/z/index.html?name=xyz#XYZ";
        mapHost = urlResolver.mapHost(uri);
        assertEquals(mapHost, "https://localhost:9096/z/index.html?name=xyz");

        uri = "https://myhost3.com:80/x/y/z/index.html?name=xyz#XYZ";
        mapHost = urlResolver.mapHost(uri);
        assertEquals(mapHost, "https://localhost:9096/a/z/index.html?name=xyz");

    }
}
