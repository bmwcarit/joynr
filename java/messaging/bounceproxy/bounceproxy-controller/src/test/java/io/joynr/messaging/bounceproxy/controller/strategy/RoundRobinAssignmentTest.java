package io.joynr.messaging.bounceproxy.controller.strategy;

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

import io.joynr.exceptions.JoynrChannelNotAssignableException;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.util.LinkedList;

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
public class RoundRobinAssignmentTest {

    private RoundRobinAssignmentStrategy assignmentStrategy;

    @Mock
    private BounceProxyDirectory directoryMock;

    @Before
    public void setUp() throws Exception {

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(BounceProxyDirectory.class).toInstance(directoryMock);
            }
        });

        assignmentStrategy = injector.getInstance(RoundRobinAssignmentStrategy.class);
    }

    @Test
    public void testAssignmentWhenBounceProxyListIsNull() {

        Mockito.when(directoryMock.getAssignableBounceProxies()).thenReturn(null);

        try {
            assignmentStrategy.calculateBounceProxy("channel-123");
            Assert.fail();
        } catch (JoynrChannelNotAssignableException e) {
            Assert.assertEquals("channel-123", e.getChannelId());
        }

    }

    @Test
    public void testAssignmentWhenBounceProxyListIsEmpty() {

        Mockito.when(directoryMock.getAssignableBounceProxies()).thenReturn(new LinkedList<BounceProxyRecord>());

        try {
            assignmentStrategy.calculateBounceProxy("channel-123");
            Assert.fail();
        } catch (JoynrChannelNotAssignableException e) {
            Assert.assertEquals("channel-123", e.getChannelId());
        }

    }

    @Test
    public void testAssignmentWithSingleBounceProxy() {

        LinkedList<BounceProxyRecord> bpList = new LinkedList<BounceProxyRecord>();
        bpList.add(createBounceProxyRecord("X.Y"));

        Mockito.when(directoryMock.getAssignableBounceProxies()).thenReturn(bpList);

        BounceProxyInformation bounceProxy = assignmentStrategy.calculateBounceProxy("channel-123");

        Assert.assertEquals("X.Y", bounceProxy.getId());
        Mockito.verify(directoryMock, Mockito.never()).updateChannelAssignment("channel-123", bounceProxy);
    }

    @Test
    public void testAssignmentToUnassignedBounceProxy() {

        LinkedList<BounceProxyRecord> bpList = new LinkedList<BounceProxyRecord>();
        bpList.add(createBounceProxyRecordWithChannels("X1.Y1", 2, 23456456456l));
        bpList.add(createBounceProxyRecordWithChannels("X2.Y2", 3, 19837453453l));
        bpList.add(createBounceProxyRecord("X3.Y3"));

        Mockito.when(directoryMock.getAssignableBounceProxies()).thenReturn(bpList);

        BounceProxyInformation bounceProxy = assignmentStrategy.calculateBounceProxy("channel-123");

        Assert.assertEquals("X3.Y3", bounceProxy.getId());
        Mockito.verify(directoryMock, Mockito.never()).updateChannelAssignment("channel-123", bounceProxy);
    }

    @Test
    public void testAssignmentToUnassignedBounceProxySmallTimestamps() {

        LinkedList<BounceProxyRecord> bpList = new LinkedList<BounceProxyRecord>();
        bpList.add(createBounceProxyRecordWithChannels("X1.Y1", 2, 2l));
        bpList.add(createBounceProxyRecordWithChannels("X2.Y2", 3, 3l));
        bpList.add(createBounceProxyRecord("X3.Y3"));

        Mockito.when(directoryMock.getAssignableBounceProxies()).thenReturn(bpList);

        BounceProxyInformation bounceProxy = assignmentStrategy.calculateBounceProxy("channel-123");

        Assert.assertEquals("X3.Y3", bounceProxy.getId());
        Mockito.verify(directoryMock, Mockito.never()).updateChannelAssignment("channel-123", bounceProxy);
    }

    @Test
    public void testAssignmentToBounceProxyWithSmallestTimestamp() {

        LinkedList<BounceProxyRecord> bpList = new LinkedList<BounceProxyRecord>();
        bpList.add(createBounceProxyRecordWithChannels("X1.Y1", 2, 3));
        bpList.add(createBounceProxyRecordWithChannels("X2.Y2", 3, 2));
        bpList.add(createBounceProxyRecordWithChannels("X3.Y3", 1, 5));

        Mockito.when(directoryMock.getAssignableBounceProxies()).thenReturn(bpList);

        BounceProxyInformation bounceProxy = assignmentStrategy.calculateBounceProxy("channel-123");

        Assert.assertEquals("X2.Y2", bounceProxy.getId());
        Mockito.verify(directoryMock, Mockito.never()).updateChannelAssignment("channel-123", bounceProxy);
    }

    private BounceProxyRecord createBounceProxyRecord(String bpId) {

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation(bpId, null);
        BounceProxyRecord record = new BounceProxyRecord(bpInfo);
        return record;
    }

    private BounceProxyRecord createBounceProxyRecordWithChannels(String bpId,
                                                                  int noOfAssignedChannels,
                                                                  long lastAssignedTimestamp) {

        BounceProxyRecord record = createBounceProxyRecord(bpId);

        for (int i = 0; i < noOfAssignedChannels; i++) {
            record.addAssignedChannel("channel-" + bpId + "-" + 1);
        }
        record.setLastAssignedTimestamp(lastAssignedTimestamp);

        return record;
    }
}
