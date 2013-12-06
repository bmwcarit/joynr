package io.joynr.messaging.bounceproxy.controller.directory;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import io.joynr.messaging.bounceproxy.controller.directory.inmemory.InMemoryBounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;

import java.util.List;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class InMemoryBounceProxyDirectoryTest {

    private BounceProxyDirectory directory;

    @Before
    public void setUp() throws Exception {
        directory = new InMemoryBounceProxyDirectory();
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
        directory.updateBounceProxyStatus("A2.B2", BounceProxyStatus.EXCLUDED);
        directory.addBounceProxy(new ControlledBounceProxyInformation("A3.B3", null));

        List<BounceProxyRecord> bounceProxies = directory.getAssignableBounceProxies();

        Assert.assertThat(bounceProxies, Matchers.hasItem(hasId("A1.B1")));
        Assert.assertThat(bounceProxies, Matchers.hasItem(hasId("A3.B3")));

        assertEquals(2, bounceProxies.size());
    }

    @Test
    public void testUpdateChannelAssignmentOnEmptyDirectory() {

        directory.updateChannelAssignment("channel-123", new BounceProxyInformation("X.Y", null));

        BounceProxyRecord bounceProxy = directory.getBounceProxy("X.Y");
        assertNull(bounceProxy);
    }

    @Test
    public void testUpdateChannelAssignmentOnNonExistingBounceProxy() {

        directory.addBounceProxy(new ControlledBounceProxyInformation("A.B", null));

        directory.updateChannelAssignment("channel-123", new BounceProxyInformation("X.Y", null));

        BounceProxyRecord bounceProxy = directory.getBounceProxy("X.Y");
        assertNull(bounceProxy);
    }

    @Test
    public void testUpdateChannelAssignment() {

        long ts = System.currentTimeMillis();
        directory.addBounceProxy(new ControlledBounceProxyInformation("X.Y", null));

        directory.updateChannelAssignment("channel-123", new BounceProxyInformation("X.Y", null));

        BounceProxyRecord bounceProxy = directory.getBounceProxy("X.Y");

        assertNotNull(bounceProxy);
        assertEquals(1, bounceProxy.getAssignedChannels());
        assertTrue(bounceProxy.getLastAssignedTimestamp() >= ts);
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
