package io.joynr.messaging.bounceproxy.controller.directory;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import io.joynr.messaging.bounceproxy.controller.directory.inmemory.InMemoryBounceProxyDirectory;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.PerformanceMeasures;
import io.joynr.messaging.info.PerformanceMeasures.Key;
import io.joynr.messaging.system.TimestampProvider;

import java.net.URI;
import java.util.List;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

@RunWith(MockitoJUnitRunner.class)
public class InMemoryBounceProxyDirectoryTest {

    private BounceProxyDirectory directory;

    @Mock
    private TimestampProvider mockTimestampProvider;

    @Before
    public void setUp() throws Exception {

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(TimestampProvider.class).toInstance(mockTimestampProvider);
                bind(BounceProxyDirectory.class).to(InMemoryBounceProxyDirectory.class);
            }
        });

        directory = injector.getInstance(BounceProxyDirectory.class);
    }

    @Test
    public void testGetAssignableBounceProxiesOnEmptyDirectory() {
        List<BounceProxyRecord> bounceProxies = directory.getAssignableBounceProxies();
        assertEquals(0, bounceProxies.size());
    }

    @Test
    public void testGetAssignableBounceProxies() {

        directory.addBounceProxy(new ControlledBounceProxyInformation("A1.B1", null));
        directory.addBounceProxy(new ControlledBounceProxyInformation("A2.B2", null));
        directory.addBounceProxy(new ControlledBounceProxyInformation("A3.B3", null));

        List<BounceProxyRecord> bounceProxies = directory.getAssignableBounceProxies();

        Assert.assertThat(bounceProxies, Matchers.hasItem(hasId("A1.B1")));
        Assert.assertThat(bounceProxies, Matchers.hasItem(hasId("A2.B2")));
        Assert.assertThat(bounceProxies, Matchers.hasItem(hasId("A3.B3")));

        assertEquals(3, bounceProxies.size());
    }

    @Test
    public void testGetAssignableBounceProxiesForDirectoryWithUnassignableOnes() {

        directory.addBounceProxy(new ControlledBounceProxyInformation("A1.B1", null));

        directory.addBounceProxy(new ControlledBounceProxyInformation("A2.B2", null));
        BounceProxyRecord bounceProxyRecord = directory.getBounceProxy("A2.B2");
        bounceProxyRecord.setStatus(BounceProxyStatus.EXCLUDED);
        directory.updateBounceProxy(bounceProxyRecord);

        directory.addBounceProxy(new ControlledBounceProxyInformation("A3.B3", null));

        List<BounceProxyRecord> bounceProxies = directory.getAssignableBounceProxies();

        Assert.assertThat(bounceProxies, Matchers.hasItem(hasId("A1.B1")));
        Assert.assertThat(bounceProxies, Matchers.hasItem(hasId("A3.B3")));

        assertEquals(2, bounceProxies.size());
    }

    @Test
    public void testUpdateChannelAssignmentOnEmptyDirectory() {

        try {
            directory.updateChannelAssignment("channel-123", new BounceProxyInformation("X.Y", null));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testUpdateChannelAssignmentOnNonExistingBounceProxy() {

        directory.addBounceProxy(new ControlledBounceProxyInformation("A.B", null));

        try {
            directory.updateChannelAssignment("channel-123", new BounceProxyInformation("X.Y", null));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testUpdateChannelAssignment() {

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(100l);
        directory.addBounceProxy(new ControlledBounceProxyInformation("X.Y", null));

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(200l);
        directory.updateChannelAssignment("channel-123", new BounceProxyInformation("X.Y", null));

        BounceProxyRecord bounceProxy = directory.getBounceProxy("X.Y");

        assertNotNull(bounceProxy);
        assertEquals(1, bounceProxy.getNumberOfAssignedChannels());
        assertEquals(200, bounceProxy.getLastAssignedTimestamp());
    }

    @Test
    public void testContainsBounceProxy() {
        directory.addBounceProxy(new ControlledBounceProxyInformation("X.Y", null));
        assertTrue(directory.containsBounceProxy("X.Y"));
    }

    @Test
    public void testDoesntContainBounceProxy() {
        assertFalse(directory.containsBounceProxy("X.Y"));
    }

    @Test
    public void testUpdateBounceProxy() {

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(100l);
        directory.addBounceProxy(new ControlledBounceProxyInformation("X.Y", URI.create("http://www.location.de")));

        BounceProxyRecord bounceProxyRecord = directory.getBounceProxy("X.Y");
        bounceProxyRecord.getInfo().setLocation(URI.create("http://www.location-updated.de"));
        bounceProxyRecord.getInfo().setLocationForBpc(URI.create("http://www.location-for-bpc-updated.de"));

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(200l);
        directory.updateBounceProxy(bounceProxyRecord);

        BounceProxyRecord result = directory.getBounceProxy("X.Y");
        assertEquals("http://www.location-updated.de", result.getInfo().getLocation().toString());
        assertEquals("http://www.location-for-bpc-updated.de", result.getInfo().getLocationForBpc().toString());
        assertEquals(200, result.getFreshness().getTime());
    }

    @Test
    public void testUpdateBounceProxyPerformance() {

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(100l);
        directory.addBounceProxy(new ControlledBounceProxyInformation("X.Y", URI.create("http://www.location.de")));

        BounceProxyRecord bounceProxyRecord = directory.getBounceProxy("X.Y");
        PerformanceMeasures performanceMeasures = new PerformanceMeasures();
        performanceMeasures.addMeasure(Key.ACTIVE_LONGPOLL_COUNT, 5);
        performanceMeasures.addMeasure(Key.ASSIGNED_CHANNELS_COUNT, 13);
        bounceProxyRecord.setPerformanceMeasures(performanceMeasures);

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(200l);
        directory.updateBounceProxy(bounceProxyRecord);

        PerformanceMeasures result = directory.getBounceProxy("X.Y").getPerformanceMeasures();
        assertEquals(5, result.getMeasure(Key.ACTIVE_LONGPOLL_COUNT));
        assertEquals(13, result.getMeasure(Key.ASSIGNED_CHANNELS_COUNT));
        assertEquals(200, directory.getBounceProxy("X.Y").getFreshness().getTime());
    }

    @Test
    public void testUpdateFreshness() {

        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(150l);
        directory.addBounceProxy(new ControlledBounceProxyInformation("X.Y", URI.create("http://www.location.de")));
        assertEquals(150, directory.getBounceProxy("X.Y").getFreshness().getTime());
    }

    private Matcher<BounceProxyRecord> hasId(final String bpId) {

        return new BaseMatcher<BounceProxyRecord>() {

            @Override
            public boolean matches(Object item) {

                BounceProxyRecord record = (BounceProxyRecord) item;
                return record.getInfo().getId().equals(bpId);
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("has BounceProxy ID '" + bpId + "'");
            }

        };
    }

}
