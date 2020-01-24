/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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
package io.joynr.messaging.bounceproxy.controller.directory.jdbc;

import java.net.URI;
import java.util.List;
import java.util.Optional;

import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.EntityTransaction;
import javax.persistence.Persistence;
import javax.persistence.Query;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.Channel;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.system.TimestampProvider;

@RunWith(MockitoJUnitRunner.class)
public class DatabasesTest {

    private static EntityManagerFactory emf;

    @Mock
    private TimestampProvider mockTimestampProvider;

    private BounceProxyDatabase bounceProxyDb;
    private ChannelDatabase channelDb;

    @BeforeClass
    public static void setUpDatabase() throws Exception {
        emf = Persistence.createEntityManagerFactory("BpcPU");
    }

    @Before
    public void setUp() {

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(EntityManagerFactory.class).toInstance(emf);
                bind(TimestampProvider.class).toInstance(mockTimestampProvider);
            }

        });

        bounceProxyDb = injector.getInstance(BounceProxyDatabase.class);
        channelDb = injector.getInstance(ChannelDatabase.class);
    }

    @After
    public void tearDown() {

        System.out.println("Tear down");

        EntityManager em = emf.createEntityManager();

        EntityTransaction tx = em.getTransaction();
        tx.begin();

        Query deleteBounceProxiesQuery = em.createQuery("DELETE FROM BounceProxyEntity");
        int deletedRows = deleteBounceProxiesQuery.executeUpdate();
        System.out.println("Deleted " + deletedRows + " bounce proxies");

        Query deleteBounceProxyInformationsQuery = em.createQuery("DELETE FROM BounceProxyInformationEntity");
        deletedRows = deleteBounceProxyInformationsQuery.executeUpdate();
        System.out.println("Deleted " + deletedRows + " bounce proxy informations");

        Query deleteChannelsQuery = em.createQuery("DELETE FROM ChannelEntity");
        deletedRows = deleteChannelsQuery.executeUpdate();

        System.out.println("Deleted " + deletedRows + " channels");

        tx.commit();
    }

    @Test
    public void testGetAssignableBounceProxiesOnEmptyDatabase() {

        List<BounceProxyRecord> assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();
        Assert.assertEquals(0, assignableBounceProxies.size());
    }

    @Test
    public void testAddBounceProxies() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(1000l);
        bounceProxyDb.addBounceProxy(bpInfo1);

        List<BounceProxyRecord> assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();
        Assert.assertEquals(1, assignableBounceProxies.size());

        Assert.assertTrue(bounceProxyDb.containsBounceProxy("bp1.0"));
        Assert.assertFalse(bounceProxyDb.containsBounceProxy("bp2.0"));
        BounceProxyRecord bpRecord1 = bounceProxyDb.getBounceProxy("bp1.0");
        Assert.assertEquals("bp1.0", bpRecord1.getBounceProxyId());
        Assert.assertEquals("http://www.joynr1.de/bp1/", bpRecord1.getInfo().getLocation().toString());
        Assert.assertEquals(0, bpRecord1.getNumberOfAssignedChannels());
        Assert.assertEquals(BounceProxyRecord.ASSIGNMENT_TIMESTAMP_NEVER, bpRecord1.getLastAssignedTimestamp());
        Assert.assertEquals(1000l, bpRecord1.getFreshness().getTime());
        Assert.assertEquals(BounceProxyStatus.ALIVE, bpRecord1.getStatus());

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("bp2.0",
                                                                                        URI.create("http://www.joynr2.de/bp2/"));

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(2000l);
        bounceProxyDb.addBounceProxy(bpInfo2);

        assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();
        Assert.assertEquals(2, assignableBounceProxies.size());

        Assert.assertTrue(bounceProxyDb.containsBounceProxy("bp1.0"));
        Assert.assertTrue(bounceProxyDb.containsBounceProxy("bp2.0"));
        BounceProxyRecord bpRecord2 = bounceProxyDb.getBounceProxy("bp2.0");
        Assert.assertEquals("bp2.0", bpRecord2.getBounceProxyId());
        Assert.assertEquals("http://www.joynr2.de/bp2/", bpRecord2.getInfo().getLocation().toString());
        Assert.assertEquals(0, bpRecord2.getNumberOfAssignedChannels());
        Assert.assertEquals(BounceProxyRecord.ASSIGNMENT_TIMESTAMP_NEVER, bpRecord2.getLastAssignedTimestamp());
        Assert.assertEquals(2000l, bpRecord2.getFreshness().getTime());
        Assert.assertEquals(BounceProxyStatus.ALIVE, bpRecord2.getStatus());
    }

    @Test
    public void testUpdateChannelAssignment() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(1000l);
        bounceProxyDb.addBounceProxy(bpInfo1);
        channelDb.addChannel(new Channel(bpInfo1,
                                         "channel-123",
                                         URI.create("http://www.joyn1.de/bp1/channels/channel-123")));

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(2000l);
        bounceProxyDb.updateChannelAssignment("channel-123", bpInfo1);

        List<BounceProxyRecord> assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();
        Assert.assertEquals(1, assignableBounceProxies.size());

        BounceProxyRecord bpRecord1 = assignableBounceProxies.get(0);
        Assert.assertEquals("bp1.0", bpRecord1.getBounceProxyId());
        Assert.assertEquals(1, bpRecord1.getNumberOfAssignedChannels());
    }

    @Test
    public void testUpdateChannelAssignmentForUnknownChannel() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));

        bounceProxyDb.addBounceProxy(bpInfo1);
        try {
            bounceProxyDb.updateChannelAssignment("channel-123", bpInfo1);
            Assert.fail();
        } catch (IllegalArgumentException e) {

        }
    }

    @Test
    public void testUpdateBounceProxy() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(1000l);
        bounceProxyDb.addBounceProxy(bpInfo1);

        BounceProxyRecord bpRecord = new BounceProxyRecord(bpInfo1);
        bpRecord.setFreshness(6000l);
        bpRecord.setLastAssignedTimestamp(4000l);
        bpRecord.setStatus(BounceProxyStatus.ACTIVE);

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(2000l);
        bounceProxyDb.updateBounceProxy(bpRecord);

        List<BounceProxyRecord> assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();
        Assert.assertEquals(1, assignableBounceProxies.size());

        BounceProxyRecord resultRecord = bounceProxyDb.getBounceProxy("bp1.0");
        Assert.assertEquals("bp1.0", resultRecord.getBounceProxyId());
        Assert.assertEquals(2000l, resultRecord.getFreshness().getTime());
        Assert.assertEquals(4000l, resultRecord.getLastAssignedTimestamp());
        Assert.assertEquals(BounceProxyStatus.ACTIVE, resultRecord.getStatus());
    }

    @Test
    public void testGetAssignableBounceProxies() {

        List<BounceProxyRecord> assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();
        Assert.assertEquals(0, assignableBounceProxies.size());

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(1000l);
        bounceProxyDb.addBounceProxy(bpInfo1);
        assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();
        Assert.assertEquals(1, assignableBounceProxies.size());

        for (BounceProxyStatus status : BounceProxyStatus.values()) {

            BounceProxyRecord bpRecord = new BounceProxyRecord(bpInfo1);
            bpRecord.setStatus(status);
            Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(2000l);
            bounceProxyDb.updateBounceProxy(bpRecord);

            assignableBounceProxies = bounceProxyDb.getAssignableBounceProxies();

            if (status.isAssignable()) {
                Assert.assertEquals(1, assignableBounceProxies.size());
            } else {
                Assert.assertEquals(0, assignableBounceProxies.size());
            }

        }
    }

    @Test
    public void testAddChannels() {

        Assert.assertEquals(0, channelDb.getChannels().size());

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(1000l);
        bounceProxyDb.addBounceProxy(bpInfo1);

        channelDb.addChannel(new Channel(bpInfo1, "channel1", URI.create("http://www.joyn1.de/bp1/channels/channel1")));

        List<Channel> channels = channelDb.getChannels();
        Assert.assertEquals(1, channels.size());

        Channel channel = channels.get(0);
        Assert.assertEquals("channel1", channel.getChannelId());
        Assert.assertEquals("bp1.0", channel.getBounceProxy().getId());
        Assert.assertEquals("http://www.joyn1.de/bp1/channels/channel1", channel.getLocation().toString());

        Assert.assertEquals(channel.getChannelId(), channelDb.getChannel(Optional.of("channel1")).getChannelId());

        channelDb.addChannel(new Channel(bpInfo1,
                                         "channel1b",
                                         URI.create("http://www.joyn1.de/bp1/channels/channel1b")));

        channels = channelDb.getChannels();
        Assert.assertEquals(2, channels.size());
        Channel channel1b = channelDb.getChannel(Optional.of("channel1b"));
        Assert.assertNotNull(channel1b);
        Assert.assertEquals("channel1b", channel1b.getChannelId());
        Assert.assertEquals("bp1.0", channel1b.getBounceProxy().getId());
        Assert.assertEquals("http://www.joyn1.de/bp1/channels/channel1b", channel1b.getLocation().toString());

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("bp2.0",
                                                                                        URI.create("http://www.joynr2.de/bp2/"));
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(2000l);
        bounceProxyDb.addBounceProxy(bpInfo2);
        channelDb.addChannel(new Channel(bpInfo2, "channel2", URI.create("http://www.joyn1.de/bp1/channels/channel2")));

        Assert.assertEquals(3, channelDb.getChannels().size());
    }

    @Test
    @Ignore
    public void testCascadingDelete() {

        Assert.assertEquals(0, channelDb.getChannels().size());

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("bp1.0",
                                                                                       URI.create("http://www.joynr1.de/bp1/"));
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(1000l);
        bounceProxyDb.addBounceProxy(bpInfo);
        channelDb.addChannel(new Channel(bpInfo, "channel1", URI.create("http://www.joyn1.de/bp1/channels/channel1")));

        Assert.assertEquals(1, bounceProxyDb.getAssignableBounceProxies().size());
        Assert.assertEquals(1, channelDb.getChannels().size());

        // delete bounce proxy
        EntityManager em = emf.createEntityManager();

        EntityTransaction tx = em.getTransaction();
        tx.begin();

        Query deleteBounceProxiesQuery = em.createQuery("DELETE FROM BounceProxyEntity");
        Assert.assertEquals(3, deleteBounceProxiesQuery.executeUpdate());

        tx.commit();

        Assert.assertEquals(0, bounceProxyDb.getAssignableBounceProxies().size());
        Assert.assertEquals(0, channelDb.getChannels().size());

        tx.begin();

        Query deleteBounceProxyInformationQuery = em.createQuery("DELETE FROM BounceProxyInformationEntity");
        Assert.assertEquals(0, deleteBounceProxyInformationQuery.executeUpdate());

        Query deleteChannelsQuery = em.createQuery("DELETE FROM ChannelEntity");
        Assert.assertEquals(0, deleteChannelsQuery.executeUpdate());

        tx.commit();
    }

    @Test
    public void testGetBounceProxyStatusInformation() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("bp1.0",
                                                                                        URI.create("http://www.joynr1.de/bp1/"));

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(1000l);
        bounceProxyDb.addBounceProxy(bpInfo1);

        List<BounceProxyStatusInformation> bpList = bounceProxyDb.getBounceProxyStatusInformation();
        Assert.assertEquals(1, bpList.size());

        BounceProxyStatusInformation bpInfo = bpList.get(0);
        Assert.assertEquals("bp1.0", bpInfo.getBounceProxyId());
        Assert.assertEquals(1000l, bpInfo.getFreshness().getTime());
        Assert.assertEquals(BounceProxyStatus.ALIVE, bpInfo.getStatus());
    }
}
