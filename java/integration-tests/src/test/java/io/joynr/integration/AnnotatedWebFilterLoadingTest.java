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

import java.io.IOException;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.annotation.WebFilter;
import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.webapp.WebAppContext;
import org.hamcrest.CoreMatchers;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.jayway.restassured.RestAssured;

public class AnnotatedWebFilterLoadingTest {

    private static String serverUrl;
    private static Server server;

    @BeforeClass
    public static void setupServer() throws Exception {
        System.setProperty("io.joynr.apps.packages", TestWebFilter.class.getPackage().getName());

        server = new Server(0);

        WebAppContext context = new WebAppContext();
        context.setWar("target/bounceproxy.war");
        context.setExtraClasspath("target/test-classes/");

        server.setHandler(context);
        server.start();
        int port = ((ServerConnector) server.getConnectors()[0]).getLocalPort();

        serverUrl = String.format("http://localhost:%d", port);
    }

    @AfterClass
    public static void stopServer() throws Exception {
        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
        System.clearProperty("io.joynr.apps.packages");
    }

    @Before
    public void setUp() {
        RestAssured.baseURI = serverUrl;
    }

    @Test
    public void testFilter() {
        RestAssured.given()
                   .expect()
                   .statusCode(200)
                   .header("test-web-filter-header", CoreMatchers.is("was-called"))
                   .get("channels");
    }

    @WebFilter
    public static class TestWebFilter implements Filter {

        @Override
        public void init(FilterConfig filterConfig) throws ServletException {
        }

        @Override
        public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException,
                                                                                                 ServletException {

            if (response instanceof HttpServletResponse) {
                HttpServletResponse httpServletResponse = (HttpServletResponse) response;
                httpServletResponse.addHeader("test-web-filter-header", "was-called");
            }
            chain.doFilter(request, response);
        }

        @Override
        public void destroy() {
        }

    }

}
