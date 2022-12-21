/*-
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.android.messaging.binder;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.concurrent.TimeUnit;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.BinderAddress;

@RunWith(AndroidJUnit4.class)
public class BinderMessagingStubTest {

    private final String testPackageName = "some.package.name";

    private BinderMessagingStub binderMessagingStub;

    @Mock
    private Context context;

    @Mock
    private BinderAddress address;

    @Mock
    private ImmutableMessage immutableMessage;

    @Mock
    private SuccessAction successAction;

    @Mock
    private FailureAction failureAction;

    private final byte[] data = "TEST".getBytes();

    private final long testLong = 1L;

    @Mock
    private TimeUnit testTimeUnit;

    @Before
    public void setup() {
        MockitoAnnotations.openMocks(this);
        binderMessagingStub = spy(new BinderMessagingStub(context, address));
    }

    @Test
    public void transmit_MessageTtlIsAbsolute_callsConnectAndTransmitData() {
        when(address.getPackageName()).thenReturn(testPackageName);
        when(immutableMessage.isTtlAbsolute()).thenReturn(true);
        binderMessagingStub.transmit(immutableMessage, successAction, failureAction);
        verify(binderMessagingStub).connectAndTransmitData(any(), eq(address), anyLong(), any(TimeUnit.class), eq(successAction), eq(failureAction));
    }

    @Test(expected = JoynrRuntimeException.class)
    public void transmit_MessageTtlIsAbsolute_ThrowsJoynrRuntimeException() {
        when(immutableMessage.isTtlAbsolute()).thenReturn(false);
        binderMessagingStub.transmit(immutableMessage, successAction, failureAction);
        verify(binderMessagingStub, never()).connectAndTransmitData(any(), eq(address), anyLong(), any(TimeUnit.class), eq(successAction), eq(failureAction));
    }

    @After
    public void cleanup() {
        reset(context);
        reset(address);
        reset(immutableMessage);
        reset(successAction);
        reset(failureAction);
    }

}
