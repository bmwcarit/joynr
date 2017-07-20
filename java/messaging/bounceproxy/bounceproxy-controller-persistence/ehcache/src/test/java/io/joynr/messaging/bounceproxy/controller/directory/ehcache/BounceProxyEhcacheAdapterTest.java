package io.joynr.messaging.bounceproxy.controller.directory.ehcache;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertNotNull;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.net.URI;
import java.util.List;

import net.sf.ehcache.CacheManager;
import static org.hamcrest.CoreMatchers.allOf;
import static org.hamcrest.Matchers.lessThanOrEqualTo;
import static org.hamcrest.Matchers.greaterThanOrEqualTo;

import org.junit.After;
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
        },
                                                 new EhcacheModule());
        cache = injector.getInstance(BounceProxyEhcacheAdapter.class);
    }

    @After
    public void tearDown() {
        CacheManager.newInstance().clearAll();
    }

    @Test
    public void testAddBounceProxy() {

        List<BounceProxyStatusInformation> list = cache.getBounceProxyStatusInformation();
        assertEquals(0, list.size());

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("bp0.0",
                                                                                       URI.create("http://www.joynr1.de/bp0.0/"));

        long earliestAddTimestamp = System.currentTimeMillis();
        cache.addBounceProxy(bpInfo);
        long latestAddTimestamp = System.currentTimeMillis();

        list = cache.getBounceProxyStatusInformation();
        assertEquals(1, list.size());
        assertTrue(cache.containsBounceProxy("bp0.0"));

        BounceProxyRecord bp0 = cache.getBounceProxy("bp0.0");
        assertEquals("bp0.0", bp0.getBounceProxyId());
        assertEquals(BounceProxyStatus.ALIVE, bp0.getStatus());
        assertThat(bp0.getFreshness().getTime(),
                   allOf(greaterThanOrEqualTo(earliestAddTimestamp), lessThanOrEqualTo(latestAddTimestamp)));

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1.0/"));
        earliestAddTimestamp = System.currentTimeMillis();
        cache.addBounceProxy(bpInfo2);
        latestAddTimestamp = System.currentTimeMillis();

        list = cache.getBounceProxyStatusInformation();
        assertEquals(2, list.size());
        assertTrue(cache.containsBounceProxy("bp0.0"));
        assertTrue(cache.containsBounceProxy("bp1.0"));
        BounceProxyRecord bp1 = cache.getBounceProxy("bp1.0");
        assertEquals("bp1.0", bp1.getBounceProxyId());
        assertEquals(BounceProxyStatus.ALIVE, bp1.getStatus());
        assertThat(bp1.getFreshness().getTime(),
                   allOf(greaterThanOrEqualTo(earliestAddTimestamp), lessThanOrEqualTo(latestAddTimestamp)));
    }

    @Test
    public void testUpdateBounceProxy() {

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("bp0.0",
                                                                                       URI.create("http://www.joynr1.de/bp0.0/"));

        long earliestAddTimestamp = System.currentTimeMillis();
        cache.addBounceProxy(bpInfo);
        long latestAddTimestamp = System.currentTimeMillis();

        List<BounceProxyStatusInformation> list = cache.getBounceProxyStatusInformation();
        assertEquals(1, list.size());

        BounceProxyStatusInformation info = cache.getBounceProxy("bp0.0");

        assertEquals("bp0.0", info.getBounceProxyId());
        assertEquals(BounceProxyStatus.ALIVE, info.getStatus());
        assertThat(info.getFreshness().getTime(),
                   allOf(greaterThanOrEqualTo(earliestAddTimestamp), lessThanOrEqualTo(latestAddTimestamp)));

        BounceProxyRecord updatedRecord = new BounceProxyRecord(bpInfo);
        updatedRecord.setStatus(BounceProxyStatus.EXCLUDED);

        long earliestUpdateTimestamp = System.currentTimeMillis();
        cache.updateBounceProxy(updatedRecord);
        long latestUpdateTimestamp = System.currentTimeMillis();

        list = cache.getBounceProxyStatusInformation();
        assertEquals(1, list.size());

        BounceProxyStatusInformation updatedInfo = cache.getBounceProxy("bp0.0");

        assertEquals("bp0.0", updatedInfo.getBounceProxyId());
        assertEquals(BounceProxyStatus.EXCLUDED, updatedInfo.getStatus());
        assertThat(updatedInfo.getFreshness().getTime(),
                   allOf(greaterThanOrEqualTo(earliestUpdateTimestamp), lessThanOrEqualTo(latestUpdateTimestamp)));
    }

    @Test
    public void testGetAssignableBounceProxiesOnEmptyCache() {
        List<BounceProxyRecord> assignableBounceProxies = cache.getAssignableBounceProxies();
        assertNotNull(assignableBounceProxies);
        assertEquals(0, assignableBounceProxies.size());
    }

    @Test
    public void testGetAssignableBounceProxies() {

        List<BounceProxyRecord> assignableBounceProxies = cache.getAssignableBounceProxies();

        assertEquals(0, assignableBounceProxies.size());

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));
        cache.addBounceProxy(bpInfo1);

        assignableBounceProxies = cache.getAssignableBounceProxies();
        assertEquals(1, assignableBounceProxies.size());

        for (BounceProxyStatus status : BounceProxyStatus.values()) {

            BounceProxyRecord bpRecord = new BounceProxyRecord(bpInfo1);
            bpRecord.setStatus(status);
            cache.updateBounceProxy(bpRecord);

            assignableBounceProxies = cache.getAssignableBounceProxies();

            if (status.isAssignable()) {
                assertEquals(1, assignableBounceProxies.size());
            } else {
                assertEquals(0, assignableBounceProxies.size());
            }
        }

    }

    @Test
    public void testUpdateChannelAssignment() {
        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));

        cache.addBounceProxy(bpInfo1);

        cache.updateChannelAssignment("channel-123", bpInfo1);

        List<BounceProxyStatusInformation> assignableBounceProxies = cache.getBounceProxyStatusInformation();
        assertEquals(1, assignableBounceProxies.size());

        BounceProxyRecord bpRecord1 = cache.getBounceProxy("bp1.0");
        assertEquals("bp1.0", bpRecord1.getBounceProxyId());
        assertEquals(1, bpRecord1.getNumberOfAssignedChannels());

        cache.updateChannelAssignment("channel-456", bpInfo1);
        bpRecord1 = cache.getBounceProxy("bp1.0");
        assertEquals("bp1.0", bpRecord1.getBounceProxyId());
        assertEquals(2, bpRecord1.getNumberOfAssignedChannels());
    }

    @Test
    public void testUpdateChannelAssignmentForAlreadyAssignedChannel() {
        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));

        cache.addBounceProxy(bpInfo1);

        cache.updateChannelAssignment("channel-123", bpInfo1);

        List<BounceProxyStatusInformation> assignableBounceProxies = cache.getBounceProxyStatusInformation();
        assertEquals(1, assignableBounceProxies.size());

        BounceProxyRecord bpRecord1 = cache.getBounceProxy("bp1.0");
        assertEquals("bp1.0", bpRecord1.getBounceProxyId());
        assertEquals(1, bpRecord1.getNumberOfAssignedChannels());

        cache.updateChannelAssignment("channel-123", bpInfo1);

        bpRecord1 = cache.getBounceProxy("bp1.0");
        assertEquals("bp1.0", bpRecord1.getBounceProxyId());
        assertEquals(1, bpRecord1.getNumberOfAssignedChannels());
    }

}
