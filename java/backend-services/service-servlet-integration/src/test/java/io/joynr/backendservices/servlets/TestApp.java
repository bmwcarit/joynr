package io.joynr.backendservices.servlets;

/*
 * #%L
 * joynr::java::backend-services::service-servlet-integration
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

import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrRuntime;

import java.util.Properties;

public class TestApp extends AbstractJoynrApplication {

    @Override
    public void run() {
        // TODO Auto-generated method stub

    }

    @Override
    public void shutdown() {
        // TODO Auto-generated method stub

    }

    public static Properties getApplicationProperties() {
        Properties properties = new Properties();
        return properties;
    }

    public JoynrRuntime getRuntime() {
        return runtime;
    }

}
