package io.joynr.messaging.bounceproxy.controller.directory.ehcache;

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

import static org.junit.Assert.assertEquals;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.net.URI;
import java.util.List;

import org.junit.Before;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

public class BounceProxyEhcacheAdapterTest {

    private BounceProxyEhcacheAdapter cache;

    @Before
    public void setUp() {

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bindConstant().annotatedWith(Names.named("joynr.bounceproxy.controller.bp_cache_name"))
                              .to("bpTestCache");
                bindConstant().annotatedWith(Names.named("joynr.bounceproxy.controller.bp_cache_config_file"))
                              .to("ehcache.xml");
            }
        }, new EhcacheModule());
        cache = injector.getInstance(BounceProxyEhcacheAdapter.class);
    }

    @Test
    public void testAddBounceProxy() {

        List<BounceProxyStatusInformation> list = cache.getBounceProxyStatusInformation();
        assertEquals(0, list.size());

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("bp0.0",
                                                                                       URI.create("http://www.joynr1.de/bp0.0/"));
        cache.addBounceProxy(bpInfo, 1000l);

        list = cache.getBounceProxyStatusInformation();
        assertEquals(1, list.size());

    }
}
