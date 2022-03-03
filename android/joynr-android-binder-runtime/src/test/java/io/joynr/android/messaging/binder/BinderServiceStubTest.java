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

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.android.messaging.binder.util.BinderConstants;
import joynr.system.RoutingTypes.BinderAddress;

@RunWith(AndroidJUnit4.class)
public class BinderServiceStubTest {

    private BinderServiceStub binderServiceStub;

    private final String testPackageName = "some.package.name";

    private final int OTHER_USER_ID = 10;

    private BinderServiceSkeleton binderService;

    private BinderServiceStub spyBinderServiceStub;

    @Mock
    private BinderServiceServer binderServiceServer;

    @Mock
    private BinderServiceClient binderServiceClient;

    @Before
    public void setup() {
        MockitoAnnotations.openMocks(this);
    }

    private void setupBinderServiceStub(int userId) {
        BinderAddress toClientAddress = new BinderAddress(testPackageName, userId);
        binderServiceStub = new BinderServiceStub(
                mock(Context.class),
                mock(Intent.class),
                mock(ServiceConnection.class),
                toClientAddress);
    }

    @Test
    public void createBinderService_userIsSystem_setsBinderServiceAsBinderServiceServer() {
        setupBinderServiceStub(BinderConstants.USER_ID_SYSTEM);
        binderServiceStub.createBinderService();

        binderService = binderServiceStub.getBinderService();
        assertTrue(binderService instanceof BinderServiceServer);
    }

    @Test
    public void createBinderService_userIsNotSystem_setsBinderServiceAsBinderServiceClient() {
        setupBinderServiceStub(OTHER_USER_ID);
        binderServiceStub.createBinderService();

        binderService = binderServiceStub.getBinderService();
        assertTrue(binderService instanceof BinderServiceClient);
    }

    @Test
    public void createBinderService_userIsSystem_bindsService(){
        setupBinderServiceStub(BinderConstants.USER_ID_SYSTEM);
        spyBinderServiceStub = spy(binderServiceStub);
        when(spyBinderServiceStub.initBinderServiceServer()).thenReturn(binderServiceServer);
        spyBinderServiceStub.createBinderService();

        verify(binderServiceServer).bindService();
    }

    @Test
    public void createBinderService_userIsNotSystem_bindsService(){
        setupBinderServiceStub(OTHER_USER_ID);
        spyBinderServiceStub = spy(binderServiceStub);
        when(spyBinderServiceStub.initBinderServiceClient()).thenReturn(binderServiceClient);
        spyBinderServiceStub.createBinderService();

        verify(binderServiceClient).bindService();
    }

    @After
    public void cleanup() {
        reset(binderServiceServer);
        reset(binderServiceClient);
    }
}
