package io.joynr.integration;

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

import io.joynr.integration.util.ServersUtil;

import org.eclipse.jetty.server.Server;
import org.junit.AfterClass;
import org.junit.BeforeClass;

import com.jayway.restassured.RestAssured;

public class BounceProxyServerTest extends AbstractBounceProxyServerTest {

    private static Server server;

    @BeforeClass
    public static void startServer() throws Exception {
        server = ServersUtil.startBounceproxy();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
    }

    public static void main(String[] args) {

        org.junit.runner.JUnitCore.main(BounceProxyServerTest.class.getName());
    }

    @Override
    protected String getBounceProxyBaseUri() {
        // using single bounce proxy only
        return RestAssured.baseURI;
    }
}
