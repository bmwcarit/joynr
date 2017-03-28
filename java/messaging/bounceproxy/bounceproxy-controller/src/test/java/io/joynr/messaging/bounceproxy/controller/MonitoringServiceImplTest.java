package io.joynr.messaging.bounceproxy.controller;

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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.PerformanceMeasures;
import io.joynr.messaging.info.PerformanceMeasures.Key;

import java.net.URI;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

@RunWith(MockitoJUnitRunner.class)
public class MonitoringServiceImplTest {

    private MonitoringServiceImpl monitoringService;

    @Mock
    BounceProxyDirectory bpDirectoryMock;

    @Before
    public void setUp() {

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(BounceProxyDirectory.class).toInstance(bpDirectoryMock);
            }

        });

        monitoringService = injector.getInstance(MonitoringServiceImpl.class);
    }

    @Test
    public void testRegisterBounceProxy() {

        Mockito.when(bpDirectoryMock.containsBounceProxy("X.Y")).thenReturn(false);

        monitoringService.register("X.Y", "http://www.joynX.de/bp", "http://joyn.some-internal-server.de/bpX");

        Mockito.verify(bpDirectoryMock)
               .addBounceProxy(new ControlledBounceProxyInformation("X.Y",
                                                                    URI.create("http://www.joynX.de/bp"),
                                                                    URI.create("http://joyn.some-internal-server.de/bpX")));
    }

    @Test
    public void testShutdownBounceProxy() {

        Mockito.when(bpDirectoryMock.containsBounceProxy("X.Y")).thenReturn(true);
        Mockito.when(bpDirectoryMock.getBounceProxy("X.Y"))
               .thenReturn(new BounceProxyRecord(new ControlledBounceProxyInformation("X.Y", null)));

        monitoringService.updateStatus("X.Y", BounceProxyStatus.SHUTDOWN);

        ArgumentCaptor<BounceProxyRecord> argument = ArgumentCaptor.forClass(BounceProxyRecord.class);
        Mockito.verify(bpDirectoryMock).updateBounceProxy(argument.capture());
        Assert.assertEquals("X.Y", argument.getValue().getBounceProxyId());
        Assert.assertEquals(BounceProxyStatus.SHUTDOWN, argument.getValue().getStatus());
    }

    @Test
    public void testGetRegisteredBounceProxies() {

        monitoringService.getRegisteredBounceProxies();

        Mockito.verify(bpDirectoryMock).getBounceProxyStatusInformation();
    }

    @Test
    public void testResetBounceProxy() {

        Mockito.when(bpDirectoryMock.containsBounceProxy("X.Y")).thenReturn(true);
        Mockito.when(bpDirectoryMock.getBounceProxy("X.Y"))
               .thenReturn(new BounceProxyRecord(new ControlledBounceProxyInformation("X.Y", null)));

        monitoringService.update("X.Y", "http://www.joynX.de/bp", "http://joyn.some-internal-server.de/bpX");

        ArgumentCaptor<BounceProxyRecord> argument = ArgumentCaptor.forClass(BounceProxyRecord.class);
        Mockito.verify(bpDirectoryMock).updateBounceProxy(argument.capture());
        Assert.assertEquals("X.Y", argument.getValue().getBounceProxyId());
        Assert.assertEquals("http://www.joynX.de/bp", argument.getValue().getInfo().getLocation().toString());
        Assert.assertEquals("http://joyn.some-internal-server.de/bpX", argument.getValue()
                                                                               .getInfo()
                                                                               .getLocationForBpc()
                                                                               .toString());
        Assert.assertEquals(BounceProxyStatus.ALIVE, argument.getValue().getStatus());
    }

    @Test
    public void testUpdatePerformanceMeasures() {

        Mockito.when(bpDirectoryMock.containsBounceProxy("X.Y")).thenReturn(true);
        Mockito.when(bpDirectoryMock.getBounceProxy("X.Y"))
               .thenReturn(new BounceProxyRecord(new ControlledBounceProxyInformation("X.Y", null)));

        PerformanceMeasures performanceMeasures = new PerformanceMeasures();
        performanceMeasures.addMeasure(Key.ACTIVE_LONGPOLL_COUNT, 5);
        monitoringService.updatePerformanceMeasures("X.Y", performanceMeasures);

        ArgumentCaptor<BounceProxyRecord> argument = ArgumentCaptor.forClass(BounceProxyRecord.class);
        Mockito.verify(bpDirectoryMock).updateBounceProxy(argument.capture());
        Assert.assertEquals("X.Y", argument.getValue().getBounceProxyId());
        Assert.assertEquals(5, argument.getValue().getPerformanceMeasures().getMeasure(Key.ACTIVE_LONGPOLL_COUNT));
        Assert.assertEquals(BounceProxyStatus.ACTIVE, argument.getValue().getStatus());
    }

    @Test
    public void testIsRegistered() {

        monitoringService.isRegistered("X.Y");

        Mockito.verify(bpDirectoryMock).containsBounceProxy("X.Y");
        Mockito.verifyNoMoreInteractions(bpDirectoryMock);
    }
}
