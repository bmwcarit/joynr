package io.joynr.messaging.bounceproxy.monitoring;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import java.net.URI;

import org.apache.http.HttpEntityEnclosingRequest;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.entity.StringEntity;
import org.apache.http.util.EntityUtils;
import org.hamcrest.Description;
import org.mockito.ArgumentMatcher;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import com.jayway.restassured.path.json.JsonPath;

public class MockitoTestUtils {

    public static HttpRequest anyPerformanceHttpRequest(final String bpId, int activeLongPolls, int assignedChannels) {
        return Mockito.argThat(new IsAnyPerformanceHttpRequest(bpId, activeLongPolls, assignedChannels));
    }

    public static ArgumentMatcher<HttpRequest> isAnyPerformanceHttpRequest(final String bpId,
                                                                           int activeLongPolls,
                                                                           int assignedChannels) {
        return new IsAnyPerformanceHttpRequest(bpId, activeLongPolls, assignedChannels);
    }

    static class IsAnyPerformanceHttpRequest extends ArgumentMatcher<HttpRequest> {

        private String bpId;
        private int activeLongPolls;
        private int assignedChannels;

        public IsAnyPerformanceHttpRequest(String bpId, int activeLongPolls, int assignedChannels) {
            this.bpId = bpId;
            this.activeLongPolls = activeLongPolls;
            this.assignedChannels = assignedChannels;
        }

        @Override
        public boolean matches(Object arg) {

            try {
                HttpRequest request = (HttpRequest) arg;

                if (!request.getRequestLine().getMethod().equals("POST")) {
                    return false;
                }

                // path parameter has to contain bpid
                URI uri = URI.create(request.getRequestLine().getUri());
                if (!uri.getPath().endsWith("/" + bpId + "/performance")) {
                    return false;
                }

                // body has to contain performance measures
                String body = EntityUtils.toString(((HttpEntityEnclosingRequest) request).getEntity());
                JsonPath json = new JsonPath(body);

                if (activeLongPolls != json.getInt("activeLongPolls")) {
                    return false;
                }

                if (assignedChannels != json.getInt("assignedChannels")) {
                    return false;
                }

                return true;

            } catch (/* URISyntax */Exception e) {
                return false;
            }
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("HTTP POST to " + bpId + "/performance");
            description.appendText(" with Json body {\"activeLongPolls\" : " + activeLongPolls
                    + ", \"assignedChannels\" : " + assignedChannels + "\"}");
        }

        @Override
        public void describeMismatch(Object item, Description description) {

            HttpRequest request = (HttpRequest) item;

            String method = request.getRequestLine().getMethod();
            String path = request.getRequestLine().getUri();
            description.appendText("HTTP " + method + " to " + path);

            // body has to contain performance measures
            try {
                String body = EntityUtils.toString(((HttpEntityEnclosingRequest) request).getEntity());
                JsonPath json = new JsonPath(body);

                int activeLongPolls = json.getInt("activeLongPolls");
                int assignedChannels = json.getInt("assignedChannels");
                description.appendText(" with Json body {\"activeLongPolls\" : " + activeLongPolls
                        + ", \"assignedChannels\" : " + assignedChannels + "\"}");
            } catch (Exception e) {
                description.appendText(" with unknown Json body (Error: " + e.getMessage() + ")");
            }

        }

    }

    public static Answer<Void> createAnswerForHttpResponse(final int httpStatus) {
        Answer<Void> answerForHttpResponse = new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {

                // We're replacing the entity by a StringEntity here, because
                // then the stream can be read as many times as desired.
                // Otherwise the entity can only be read once, which causes
                // problems. Somewhere it is obviously read before it is
                // unpacked in the test assertion.
                HttpEntityEnclosingRequest httpRequest = (HttpEntityEnclosingRequest) invocation.getArguments()[0];
                String body = EntityUtils.toString(httpRequest.getEntity());
                httpRequest.setEntity(new StringEntity(body));

                HttpResponse httpResponse = (HttpResponse) invocation.getArguments()[1];
                httpResponse.setStatusCode(httpStatus);
                return null;
            }
        };
        return answerForHttpResponse;
    }
}
