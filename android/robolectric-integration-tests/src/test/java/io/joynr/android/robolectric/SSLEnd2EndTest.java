/*
 * #%L
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
package io.joynr.android.robolectric;

import java.util.Properties;

import org.junit.Before;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

import com.google.inject.Module;

import io.joynr.android.test.TestActivity;
import io.joynr.integration.AbstractSSLEnd2EndTest;
import io.joynr.runtime.JoynrRuntime;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = "./src/test/AndroidManifest.xml")
public class SSLEnd2EndTest extends AbstractSSLEnd2EndTest {

    private TestActivity activity;

    @Before
    public void robolectricSetup() throws Exception {

        // Uncomment to log the verbose android logs to stdout
        //ShadowLog.stream = System.out;
    }

    @Override
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        if (activity == null) {
            activity = Robolectric.buildActivity(TestActivity.class).create().get();
        }

        return activity.createRuntime(joynrConfig, modules);
    }
}
