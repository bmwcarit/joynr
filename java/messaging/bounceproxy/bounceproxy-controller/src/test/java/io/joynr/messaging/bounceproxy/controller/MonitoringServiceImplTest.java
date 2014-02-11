package io.joynr.messaging.bounceproxy.controller;

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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.bounceproxy.controller.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;

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

        monitoringService.register("X.Y", "http://www.joynX.de/bp", "http://joyn.bmwgroup.net/bpX");

        ArgumentCaptor<ControlledBounceProxyInformation> argument = ArgumentCaptor.forClass(ControlledBounceProxyInformation.class);
        Mockito.verify(bpDirectoryMock).addBounceProxy(argument.capture(), Mockito.anyLong());
        Assert.assertEquals("X.Y", argument.getValue().getId());
        Assert.assertEquals(URI.create("http://www.joynX.de/bp"), argument.getValue().getLocation());
        Assert.assertEquals(URI.create("http://joyn.bmwgroup.net/bpX"), argument.getValue().getLocationForBpc());
    }

    @Test
    public void testShutdownBounceProxy() {

        Mockito.when(bpDirectoryMock.containsBounceProxy("X.Y")).thenReturn(true);
        Mockito.when(bpDirectoryMock.getBounceProxy("X.Y"))
               .thenReturn(new BounceProxyRecord(new ControlledBounceProxyInformation("X.Y", null)));

        monitoringService.updateStatus("X.Y", BounceProxyStatus.SHUTDOWN);

        ArgumentCaptor<BounceProxyRecord> argument = ArgumentCaptor.forClass(BounceProxyRecord.class);
        Mockito.verify(bpDirectoryMock).updateBounceProxy(argument.capture(), Mockito.anyLong());
        Assert.assertEquals("X.Y", argument.getValue().getBounceProxyId());
        Assert.assertEquals(BounceProxyStatus.SHUTDOWN, argument.getValue().getStatus());
    }
}
