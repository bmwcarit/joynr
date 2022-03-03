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
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;

import android.content.ComponentName;
import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;

@RunWith(AndroidJUnit4.class)
public class BinderServiceConnectionTest {

    private BinderServiceConnection binderServiceConnection;

    @Mock
    private Context context;

    @Mock
    private SuccessAction successAction;

    @Mock
    private FailureAction failureAction;

    @Mock
    private io.joynr.android.messaging.binder.JoynrBinder binder;

    private final byte[] data = "TEST".getBytes();

    @Before
    public void setup() {
        MockitoAnnotations.openMocks(this);

    }

    @Test
    public void onServiceConnected_whenBinderTransmitSucceeds_unbindsServiceAndExecutesSuccessAction() {
        binderServiceConnection = new BinderServiceConnection(context, data, successAction, failureAction);
        binderServiceConnection.onServiceConnected(mock(ComponentName.class), mock(IBinder.class));
        verify(context).unbindService(any());
        verify(successAction).execute();
    }

    @Test
    public void onServiceConnected_whenBinderThrowsRemoteException_unbindsServiceAndExecutesFailureAction() throws RemoteException {
        binderServiceConnection = new BinderServiceConnection(context, data, successAction, failureAction);

        doThrow(RemoteException.class).when(binder).transmit(any());

        MockedStatic<JoynrBinder.Stub> joynrBinderStub = Mockito.mockStatic(JoynrBinder.Stub.class);
        joynrBinderStub.when(() -> io.joynr.android.messaging.binder.JoynrBinder.Stub.asInterface(any())).thenReturn(binder);

        binderServiceConnection.onServiceConnected(mock(ComponentName.class), mock(IBinder.class));
        verify(context).unbindService(any());
        verify(failureAction).execute(any());

        joynrBinderStub.close();
    }

    @Test
    public void onServiceDisconnected_executesFailureAction() {
        binderServiceConnection = new BinderServiceConnection(context, data, successAction, failureAction);
        binderServiceConnection.onServiceDisconnected(mock(ComponentName.class));
        verify(failureAction).execute(any());
    }

    @After
    public void cleanup() {
        reset(context);
        reset(successAction);
        reset(failureAction);
        reset(binder);
    }
}
