/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.android;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.when;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.google.inject.Injector;
import com.google.inject.Module;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.Properties;

import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;

@RunWith(AndroidJUnit4.class)
public class AndroidBinderRuntimeTest {

    private String testClusterControllerPackageName;

    @Mock
    private Context context;

    private String testBrokerUri;

    private Properties testProperties;

    private Module testModule1;
    private Module testModule2;

    @Mock
    private Injector injector;

    @Mock
    private JoynrRuntime joynrRuntime;

    @Mock
    private JoynrInjectorFactory joynrInjectorFactory;

    MockedStatic<AndroidBinderRuntimeUtils> androidBinderRuntimeUtilsMockedStatic;

    @Before
    public void setup() {

        MockitoAnnotations.openMocks(this);

        testClusterControllerPackageName = "com.bmwgroup.apinext.joynrclustercontroller";

        testBrokerUri = "mqtts://mqtt.e2e.cd-emea.bmw:8884";

        testProperties = new Properties();

        testModule1 = binder -> {};

        testModule2 = binder -> {};

        androidBinderRuntimeUtilsMockedStatic = Mockito.mockStatic(AndroidBinderRuntimeUtils.class);

    }

    @Test
    public void initClusterController_whenNoModulesInArguments_variablesAreInitialized() {

        when(injector.getInstance(JoynrRuntime.class)).thenReturn(joynrRuntime);
        when(joynrInjectorFactory.getInjector()).thenReturn(injector);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getJoynrInjectorFactory(any(), any())).thenReturn(joynrInjectorFactory);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getDefaultJoynrProperties(context)).thenReturn(testProperties);

        JoynrRuntime runtime = AndroidBinderRuntime.initClusterController(context, testBrokerUri, testProperties);

        assertEquals(injector, AndroidBinderRuntime.getInjector());
        assertEquals(joynrRuntime, runtime);

    }

    @Test
    public void initClusterController_whenOneModuleInArguments_variablesAreInitialized() {

        when(injector.getInstance(JoynrRuntime.class)).thenReturn(joynrRuntime);
        when(joynrInjectorFactory.getInjector()).thenReturn(injector);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getJoynrInjectorFactory(any(), any())).thenReturn(joynrInjectorFactory);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getDefaultJoynrProperties(context)).thenReturn(testProperties);

        JoynrRuntime runtime = AndroidBinderRuntime.initClusterController(context, testBrokerUri, testProperties, testModule1);

        assertEquals(injector, AndroidBinderRuntime.getInjector());
        assertEquals(joynrRuntime, runtime);

    }

    @Test
    public void initClusterController_whenTwoModuledInArguments_variablesAreInitialized() {

        when(injector.getInstance(JoynrRuntime.class)).thenReturn(joynrRuntime);
        when(joynrInjectorFactory.getInjector()).thenReturn(injector);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getJoynrInjectorFactory(any(), any())).thenReturn(joynrInjectorFactory);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getDefaultJoynrProperties(context)).thenReturn(testProperties);

        JoynrRuntime runtime = AndroidBinderRuntime.initClusterController(context, testBrokerUri, testProperties, testModule1, testModule2);

        assertEquals(injector, AndroidBinderRuntime.getInjector());
        assertEquals(joynrRuntime, runtime);

    }


    @Test
    public void init_whenCalledWithOnlyContext_variablesAreInitialized() {

        when(injector.getInstance(JoynrRuntime.class)).thenReturn(joynrRuntime);
        when(joynrInjectorFactory.createChildInjector()).thenReturn(injector);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getClusterControllerServicePackageName(context)).thenReturn(testClusterControllerPackageName);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getJoynrInjectorFactory(any(), any())).thenReturn(joynrInjectorFactory);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getDefaultJoynrProperties(context)).thenReturn(testProperties);

        JoynrRuntime runtime = AndroidBinderRuntime.init(context);

        assertEquals(injector, AndroidBinderRuntime.getInjector());
        assertEquals(joynrRuntime, runtime);

    }

    @Test
    public void init_whenCalledWithProperties_variablesAreInitialized() {

        when(injector.getInstance(JoynrRuntime.class)).thenReturn(joynrRuntime);
        when(joynrInjectorFactory.createChildInjector()).thenReturn(injector);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getClusterControllerServicePackageName(context)).thenReturn(testClusterControllerPackageName);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getJoynrInjectorFactory(any(), any())).thenReturn(joynrInjectorFactory);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getDefaultJoynrProperties(context)).thenReturn(testProperties);

        JoynrRuntime runtime = AndroidBinderRuntime.init(context, testProperties);

        assertEquals(injector, AndroidBinderRuntime.getInjector());
        assertEquals(joynrRuntime, runtime);

    }

    @Test
    public void init_whenClusterControllerPackageNameNotFound_runtimeIsNull() {

        when(injector.getInstance(JoynrRuntime.class)).thenReturn(joynrRuntime);
        when(joynrInjectorFactory.createChildInjector()).thenReturn(injector);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getClusterControllerServicePackageName(context)).thenReturn(null);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getJoynrInjectorFactory(any(), any())).thenReturn(joynrInjectorFactory);
        androidBinderRuntimeUtilsMockedStatic.when(() -> AndroidBinderRuntimeUtils.getDefaultJoynrProperties(context)).thenReturn(testProperties);

        JoynrRuntime runtime = AndroidBinderRuntime.init(context, testProperties);

        assertNull(runtime);
    }

    @After
    public void cleanup() {
        AndroidBinderRuntime.setRuntime(null);
        AndroidBinderRuntime.setInjector(null);
        androidBinderRuntimeUtilsMockedStatic.close();
        reset(context);
        reset(injector);
        reset(joynrRuntime);
        reset(joynrInjectorFactory);
    }
}
