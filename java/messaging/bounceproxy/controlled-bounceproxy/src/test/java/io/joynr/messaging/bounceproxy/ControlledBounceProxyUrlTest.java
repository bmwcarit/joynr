package io.joynr.messaging.bounceproxy;

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

import org.junit.Assert;
import org.junit.Test;

public class ControlledBounceProxyUrlTest {

    @Test
    public void testCreationWithoutSlash() {

        ControlledBounceProxyUrl url = new ControlledBounceProxyUrl("http://www.joyn.de:8080",
                                                                    "http://joyn-internal.intra");

        Assert.assertEquals("http://www.joyn.de:8080/", url.getOwnUrlForClusterControllers());
        Assert.assertEquals("http://joyn-internal.intra/", url.getOwnUrlForBounceProxyController());
    }

    @Test
    public void testCreationWithSlash() {

        ControlledBounceProxyUrl url = new ControlledBounceProxyUrl("http://www.joyn.de:8080/",
                                                                    "http://joyn-internal.intra/");

        Assert.assertEquals("http://www.joyn.de:8080/", url.getOwnUrlForClusterControllers());
        Assert.assertEquals("http://joyn-internal.intra/", url.getOwnUrlForBounceProxyController());
    }
}
