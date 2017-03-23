package io.joynr.messaging.bounceproxy;

/*
 * #%L
 * joynr::java::messaging::service-common
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
import java.util.List;

import org.apache.http.Header;
import org.apache.http.HttpRequest;
import org.apache.http.NameValuePair;
import org.apache.http.client.utils.URLEncodedUtils;
import org.hamcrest.Description;
import org.mockito.ArgumentMatcher;

public class IsCreateChannelHttpRequest extends ArgumentMatcher<HttpRequest> {

    private String ccid;
    private String trackingId;

    public IsCreateChannelHttpRequest(String ccid, String trackingId) {
        this.ccid = ccid;
        this.trackingId = trackingId;
    }

    @Override
    public boolean matches(Object argument) {

        HttpRequest request = (HttpRequest) argument;

        // check if tracking ID is sent in header
        Header trackingIdHeader = request.getFirstHeader("X-Atmosphere-tracking-id");
        if (trackingIdHeader == null) {
            // no tracking ID header set at all
            return false;
        } else {
            if (!trackingIdHeader.getValue().equals(trackingId)) {
                // wrong tracking ID header set
                return false;
            }
        }

        // check if channel ID is sent as query parameter ccid
        List<NameValuePair> queryParameters = URLEncodedUtils.parse(URI.create(request.getRequestLine().getUri()),
                                                                    "UTF-8");
        for (NameValuePair queryParameter : queryParameters) {
            if (queryParameter.getName().equals("ccid") && queryParameter.getValue().equals(ccid)) {
                // right channel ID sent
                return true;
            } else {
                // wrong channel ID sent
                return false;
            }
        }
        // no query parameter with ccid sent at all
        return false;
    }

    @Override
    public void describeTo(Description description) {
        description.appendText("HTTP POST with query parameter ccid=" + ccid + " and header X-Atmosphere-tracking-id="
                + trackingId);
    }

}