package io.joynr.joynrandroidruntime.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.messaging.httpoperation.HttpDelete;
import io.joynr.messaging.httpoperation.HttpGet;
import io.joynr.messaging.httpoperation.HttpPost;
import io.joynr.messaging.httpoperation.HttpRequestFactory;

import java.net.URI;

public class ApacheAndroidHttpRequestFactory implements HttpRequestFactory {
    public HttpDelete createHttpDelete(URI uri) {
        return new ApacheAndroidHttpDelete(uri);
    }

    public HttpGet createHttpGet(URI uri) {
        return new ApacheAndroidHttpGet(uri);
    }

    public HttpPost createHttpPost(URI uri) {
        return new ApacheAndroidHttpPost(uri);
    }
}
