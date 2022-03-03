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

import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.when;

import static org.junit.Assert.assertEquals;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class AndroidBinderRuntimeUtilsTest {

    private String testClusterControllerPackageName;

    @Mock
    private Context context;

    @Mock
    private PackageManager packageManager;

    @Mock
    private ResolveInfo resolveInfo;
    List<ResolveInfo> resolveInfoList;

    @Mock
    private ServiceInfo serviceInfo;

    @Mock
    private ApplicationInfo applicationInfo;

    @Before
    public void setup() {

        testClusterControllerPackageName = "com.bmwgroup.apinext.joynrclustercontroller";

        MockitoAnnotations.openMocks(this);

        resolveInfoList = new ArrayList<>();
    }

    @Test
    public void getClusterControllerServicePackageName_whenResolveInfoListHasElements_returnsCCName() {

        resolveInfoList.add(resolveInfo);
        applicationInfo.packageName = testClusterControllerPackageName;
        serviceInfo.applicationInfo = applicationInfo;
        resolveInfo.serviceInfo = serviceInfo;
        when(packageManager.queryIntentServices(any(), anyInt())).thenReturn(resolveInfoList);
        when(context.getPackageManager()).thenReturn(packageManager);
        String clusterControllerName = AndroidBinderRuntimeUtils.getClusterControllerServicePackageName(context);
        assertEquals(testClusterControllerPackageName, clusterControllerName);
    }

    @Test
    public void getClusterControllerServicePackageName_whenResolveInfoListIsNull_returnsNull() {

        when(packageManager.queryIntentServices(any(), anyInt())).thenReturn(null);
        when(context.getPackageManager()).thenReturn(packageManager);
        String clusterControllerName = AndroidBinderRuntimeUtils.getClusterControllerServicePackageName(context);
        assertNull(clusterControllerName);
    }

    @Test
    public void getClusterControllerServicePackageName_whenResolveInfoListIsEmpty_returnsNull() {

        when(packageManager.queryIntentServices(any(), anyInt())).thenReturn(resolveInfoList);
        when(context.getPackageManager()).thenReturn(packageManager);
        String clusterControllerName = AndroidBinderRuntimeUtils.getClusterControllerServicePackageName(context);
        assertNull(clusterControllerName);
    }

    @After
    public void cleanup() {
        reset(context);
        reset(packageManager);
        reset(resolveInfo);
        reset(serviceInfo);
        reset(applicationInfo);
    }

}
