/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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
package io.joynr.proxy;

import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.exceptions.JoynrIllegalStateException;
import joynr.vehicle.NavigationStatelessAsyncCallback;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import static io.joynr.proxy.StatelessAsyncIdCalculator.USE_CASE_SEPARATOR;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(MockitoJUnitRunner.class)
public class StatelessAsyncCallbackDirectoryTest {

    private static final String USE_CASE = "test";

    private static final String CALLBACK_ID = "callbackId" + USE_CASE_SEPARATOR + USE_CASE;

    @Mock
    private ReplyCallerDirectory replyCallerDirectoryMock;

    @Mock
    private StatelessAsyncIdCalculator statelessAsyncIdCalculatorMock;

    @InjectMocks
    private StatelessAsyncCallbackDirectory subject;

    @Before
    public void setup() {
        when(statelessAsyncIdCalculatorMock.calculateStatelessCallbackId(anyString(), any())).thenReturn(CALLBACK_ID);
    }

    class MyTestCallback implements NavigationStatelessAsyncCallback {

        @Override
        public String getUseCase() {
            return USE_CASE;
        }

    }

    @Test
    public void testSuccessfullyRegisterCallback() {
        subject.register(new MyTestCallback());
        ArgumentCaptor<ReplyCaller> captor = ArgumentCaptor.forClass(ReplyCaller.class);
        verify(replyCallerDirectoryMock).addReplyCaller(eq(CALLBACK_ID), captor.capture(), any());
        ReplyCaller replyCaller = captor.getValue();
        assertNotNull(replyCaller);
        assertTrue(replyCaller instanceof StatelessAsyncReplyCaller);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailsForDuplicateRegistration() {
        subject.register(new MyTestCallback());
        subject.register(new MyTestCallback());
    }

    @Test
    public void testGetRegisteredCallback() {
        MyTestCallback callback = new MyTestCallback();
        subject.register(callback);
        StatelessAsyncCallback result = subject.get(USE_CASE);
        assertEquals(callback, result);
    }
}
