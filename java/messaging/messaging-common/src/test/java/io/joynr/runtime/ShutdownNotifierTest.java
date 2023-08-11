/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.runtime;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class ShutdownNotifierTest {

    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Before
    public void setup() {
        shutdownNotifier = new ShutdownNotifier();
    }

    @Test(expected = IllegalArgumentException.class)
    public void registerIncorrectMessageTrackerShutdownListener() {
        shutdownNotifier.registerMessageTrackerShutdownListener(() -> {
        });
    }

    @Test(expected = IllegalArgumentException.class)
    public void registerIncorrectMessageTrackerPrepareForShutdownListener() {
        shutdownNotifier.registerMessageTrackerPrepareForShutdownListener(new PrepareForShutdownListener() {
            @Override
            public void prepareForShutdown() {
                // no-op
            }
        });
    }

    @Test(expected = IllegalArgumentException.class)
    public void registerIncorrectProxyInvocationHandlerShutdownListener() {
        shutdownNotifier.registerProxyInvocationHandlerShutdownListener(() -> {
        });
    }

    @Test(expected = IllegalArgumentException.class)
    public void registerIncorrectProxyInvocationHandlerPrepareForShutdownListener() {
        shutdownNotifier.registerProxyInvocationHandlerPrepareForShutdownListener(new PrepareForShutdownListener() {
            @Override
            public void prepareForShutdown() {
                // no-op
            }
        });
    }

    @Test(expected = IllegalArgumentException.class)
    public void registerIncorrectHivemqMqttShutdownListener() {
        shutdownNotifier.registerHivemqMqttShutdownListener(() -> {
        });
    }

    @Test(expected = IllegalArgumentException.class)
    public void registerIncorrectHivemqMqttPrepareForShutdownListener() {
        shutdownNotifier.registerHivemqMqttPrepareForShutdownListener(new PrepareForShutdownListener() {
            @Override
            public void prepareForShutdown() {
                // no-op
            }
        });
    }
}
