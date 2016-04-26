package io.joynr.proxy;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

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

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;

@RunWith(MockitoJUnitRunner.class)
public class ProxyInvocationHandlerTest {
    private MessagingQos messagingQos = new MessagingQos();
    private DiscoveryQos discoveryQos = new DiscoveryQos();
    private String proxyParticipantId = "proxyParticipantId";
    private String interfaceName = "interfaceName";
    private String domain = "domain";

    private final ExecutorService threadPool = new ScheduledThreadPoolExecutor(2);

    public static interface TestSyncInterface {
        public void testMethod();
    }

    @Before
    public void setUp() throws Exception {
    }

    @Test(timeout = 3000)
    public void callProxyInvocationHandlerSyncFromMultipleThreadsTest() throws Throwable {

        ConnectorFactory connectorFactory = Mockito.mock(ConnectorFactory.class);
        final ProxyInvocationHandlerImpl proxyInvocationHandler = new ProxyInvocationHandlerImpl(domain,
                                                                                                 interfaceName,
                                                                                                 proxyParticipantId,
                                                                                                 discoveryQos,
                                                                                                 messagingQos,
                                                                                                 connectorFactory);

        Future<?> call1 = threadPool.submit(new Callable<Object>() {
            @Override
            public Object call() throws Exception {
                Object result = null;
                try {
                    result = proxyInvocationHandler.invoke(TestSyncInterface.class.getDeclaredMethod("testMethod",
                                                                                                     new Class<?>[]{}),
                                                           new Object[]{});
                } catch (Throwable e) {
                }

                return result;
            }
        });

        Future<?> call2 = threadPool.submit(new Callable<Object>() {
            @Override
            public Object call() throws Exception {
                Object result = null;
                try {
                    result = proxyInvocationHandler.invoke(TestSyncInterface.class.getDeclaredMethod("testMethod",
                                                                                                     new Class<?>[]{}),
                                                           new Object[]{});
                } catch (Throwable e) {
                }
                return result;
            }
        });

        proxyInvocationHandler.createConnector(new ArbitrationResult("participantId"));

        // if the bug that causes one thread to hang in arbitration exists, one
        // of these calls will never return, causing the test to timeout and fail
        call1.get();
        call2.get();

    }
}
