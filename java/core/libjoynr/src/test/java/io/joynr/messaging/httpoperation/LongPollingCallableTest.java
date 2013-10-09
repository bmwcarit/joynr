package io.joynr.messaging.httpoperation;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import java.io.IOException;
import java.util.UUID;

import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.localserver.LocalTestServer;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestHandler;
import org.junit.After;
import org.junit.Before;

public class LongPollingCallableTest {

    private LocalTestServer server;

    private static final String CHANNELPATH = "/bounceproxy/channels/";

    private String bounceProxyUrl;
    private String channelId = "LongPollingCallableTest_" + UUID.randomUUID().toString();

    private String serviceAddress;

    @Before
    public void setUp() throws Exception {
        server = new LocalTestServer(null, null);
        server.register(CHANNELPATH, new HttpRequestHandler() {
            @Override
            public void handle(HttpRequest request, HttpResponse response, HttpContext context) throws HttpException,
                                                                                               IOException {
                response.setStatusCode(HttpStatus.SC_CREATED);
                response.setHeader("Location", bounceProxyUrl + channelId);
                server.register(CHANNELPATH + channelId, new HttpRequestHandler() {
                    @Override
                    public void handle(HttpRequest newRequest, HttpResponse newResponse, HttpContext newContext)
                                                                                                                throws HttpException,
                                                                                                                IOException {
                        newResponse.setStatusCode(HttpStatus.SC_CREATED);
                        newResponse.setHeader("Location", bounceProxyUrl + channelId);
                    }
                });
            }
        });
        server.start();
        serviceAddress = "http://" + server.getServiceAddress().getHostName() + ":"
                + server.getServiceAddress().getPort();
        bounceProxyUrl = serviceAddress + CHANNELPATH;
    }

    @After
    public void tearDown() throws Exception {
        server.stop();
    }
}
