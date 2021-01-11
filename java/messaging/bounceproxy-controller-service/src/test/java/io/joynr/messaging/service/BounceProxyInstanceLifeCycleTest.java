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
package io.joynr.messaging.service;

import static com.jayway.restassured.RestAssured.given;
import static io.joynr.messaging.datatypes.JoynrBounceProxyControlErrorCode.BOUNCEPROXY_STATUS_UNKNOWN;
import static io.joynr.messaging.datatypes.JoynrBounceProxyControlErrorCode.BOUNCEPROXY_UNKNOWN;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.servlet.ServletModule;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.PerformanceMeasures;

/**
 * Tests the sequences of the sequence diagram "BounceProxy Instance Life Cycle"
 * 
 * @author christina.strobel
 * 
 */
@RunWith(MockitoJUnitRunner.class)
public class BounceProxyInstanceLifeCycleTest extends AbstractServiceInterfaceTest {

    private String serverUrl;

    @Mock
    private MonitoringService mock;

    @Override
    protected ServletModule getServletTestModule() {

        return new ServletModule() {

            @Override
            protected void configureServlets() {

                bind(MonitoringServiceRestAdapter.class);
                bind(MonitoringService.class).toInstance(mock);

                serve("/*").with(GuiceContainer.class);
            }

        };
    }

    @Before
    public void setUp() throws Exception {
        super.setUp();

        serverUrl = String.format("%s/bounceproxies/", getServerUrlWithoutPath());
    }

    @Test
    public void testStartBounceProxyThatIsAlreadyKnown() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(true);

        Response response = //
                given() //
                       .queryParam("url4cc", "http://testurl/url4cc")
                       .and()
                       .queryParam("url4bpc", "http://testurl/url4bpc")
                       //
                       .when()
                       //
                       .put(serverUrl + "?bpid=0.0");

        assertEquals(204 /* No Content */, response.getStatusCode());
        assertNull(response.getHeader("Location"));
        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verify(mock).update("0.0", "http://testurl/url4cc", "http://testurl/url4bpc");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testStartNewBounceProxy() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(false);

        Response response = //
                given() //
                       .queryParam("url4cc", "http://testurl/url4cc")
                       //
                       .and()
                       //
                       .queryParam("url4bpc", "http://testurl/url4bpc")
                       //
                       .when()
                       //
                       .put(serverUrl + "?bpid=0.0");

        assertEquals(201 /* Created */, response.getStatusCode());
        assertEquals("http://testurl/url4bpc", response.getHeader("Location"));
        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verify(mock).register("0.0", "http://testurl/url4cc", "http://testurl/url4bpc");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testReportPerformance() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(true);

        Response response = //
                given() //
                       .when()
                       //
                       .contentType(ContentType.JSON)
                       //
                       .body("{\"activeLongPolls\":5}")
                       //
                       .post(serverUrl + "0.0/performance");

        assertEquals(204 /* No Content */, response.getStatusCode());
        Mockito.verify(mock)
               .updatePerformanceMeasures("0.0",
                                          new MeasureBuilder().add(PerformanceMeasures.Key.ACTIVE_LONGPOLL_COUNT, 5)
                                                              .build());
        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testReportPerformanceForUnknownBounceProxy() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(false);

        Response response = //
                given() //
                       .when()
                       //
                       .contentType(ContentType.JSON)
                       //
                       .body("{\"activeLongPolls\":5}")
                       //
                       .post(serverUrl + "0.0/performance");

        assertEquals(400 /* Bad Request */, response.getStatusCode());
        assertEquals(String.format("{\"_typeName\":\"Error\",\"code\":%d,\"reason\":\"%s: bounce proxy '0.0'\"}",
                                   BOUNCEPROXY_UNKNOWN.getCode(),
                                   BOUNCEPROXY_UNKNOWN.getDescription()),
                     response.getBody().asString());
        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testUpdateStatusWithUnknownStatus() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(true);

        Response response = //
                given().when().put(serverUrl + "0.0/lifecycle?status=blablabla");

        assertEquals(400 /* Bad Request */, response.getStatusCode());
        assertEquals(String.format("{\"_typeName\":\"Error\",\"code\":%d,\"reason\":\"%s: status 'blablabla'\"}",
                                   BOUNCEPROXY_STATUS_UNKNOWN.getCode(),
                                   BOUNCEPROXY_STATUS_UNKNOWN.getDescription()),
                     response.getBody().asString());
        Mockito.verifyZeroInteractions(mock);
    }

    @Test
    public void testReportPerformanceWithUnknownMeasureName() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(true);

        Response response = //
                given() //
                       .when()
                       //
                       .contentType(ContentType.JSON)
                       //
                       .body("{ \"active\" : 5 }")
                       //
                       .post(serverUrl + "0.0/performance");

        assertEquals(204 /* No Content */, response.getStatusCode());
        Mockito.verify(mock).updatePerformanceMeasures("0.0", new PerformanceMeasures());
        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testReportPerformanceWithMultipleMeasures() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(true);

        Response response = //
                given(). //
                       when()
                       .contentType(ContentType.JSON)
                       .body("{ \"activeLongPolls\" : 5, \"assignedChannels\" : 3 }")
                       .post(serverUrl + "0.0/performance");

        assertEquals(204 /* No Content */, response.getStatusCode());
        Mockito.verify(mock)
               .updatePerformanceMeasures("0.0",
                                          new MeasureBuilder().add(PerformanceMeasures.Key.ACTIVE_LONGPOLL_COUNT, 5)
                                                              .add(PerformanceMeasures.Key.ASSIGNED_CHANNELS_COUNT, 3)
                                                              .build());
        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testShutdownBounceProxyInstance() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(true);

        Response response = //
                given().when().put(serverUrl + "0.0/lifecycle?status=shutdown");

        assertEquals(204 /* No Content */, response.getStatusCode());
        Mockito.verify(mock).updateStatus("0.0", BounceProxyStatus.SHUTDOWN);
        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testShutdownUnknownBounceProxyInstance() {

        Mockito.when(mock.isRegistered("0.0")).thenReturn(false);

        Response response = //
                given().when().put(serverUrl + "0.0/lifecycle?status=shutdown");

        assertEquals(400 /* Bad Request */, response.getStatusCode());
        assertEquals(String.format("{\"_typeName\":\"Error\",\"code\":%d,\"reason\":\"%s: bounce proxy '0.0'\"}",
                                   BOUNCEPROXY_UNKNOWN.getCode(),
                                   BOUNCEPROXY_UNKNOWN.getDescription()),
                     response.getBody().asString());

        Mockito.verify(mock).isRegistered("0.0");
        Mockito.verifyNoMoreInteractions(mock);
    }

    class MeasureBuilder {

        private PerformanceMeasures measure = new PerformanceMeasures();

        public MeasureBuilder add(PerformanceMeasures.Key key, int value) {
            measure.addMeasure(key, value);
            return this;
        }

        public PerformanceMeasures build() {
            return measure;
        }
    }

}
