package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::bounceproxy-controller-service
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

import static com.jayway.restassured.RestAssured.given;
import static org.junit.Assert.assertEquals;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.servlet.ServletModule;
import com.jayway.restassured.response.Response;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

@RunWith(MockitoJUnitRunner.class)
public class BounceProxyMigrationTest extends AbstractServiceInterfaceTest {

    private String serverUrl;

    @Mock
    MigrationService mock;

    @Override
    protected ServletModule getServletTestModule() {

        return new ServletModule() {

            @Override
            protected void configureServlets() {

                bind(MigrationService.class).toInstance(mock);
                bind(MigrationServiceRestAdapter.class);

                serve("/*").with(GuiceContainer.class);
            }

        };
    }

    @Before
    public void setUp() throws Exception {
        super.setUp();

        serverUrl = String.format("%s/migration", getServerUrlWithoutPath());
    }

    @Test
    public void testClusterMigration() {

        Response response = //
        given(). //
        when()
               .delete(serverUrl + "/clusters/cluster0");

        assertEquals(202 /* Accepted */, response.getStatusCode());
        Mockito.verify(mock).startClusterMigration("cluster0");
    }

    @Test
    public void testBounceProxyMigration() {

        Response response = //
        given(). //
        when()
               .delete(serverUrl + "/bps/cluster0.instance0");

        assertEquals(501 /* Not Implemented */, response.getStatusCode());
        Mockito.verifyZeroInteractions(mock);
    }

    @Test
    public void testChannelMigration() {

        Response response = //
        given(). //
        when()
               .delete(serverUrl + "/channels/channel-123");

        assertEquals(501 /* Not Implemented */, response.getStatusCode());
        Mockito.verifyZeroInteractions(mock);
    }
}
