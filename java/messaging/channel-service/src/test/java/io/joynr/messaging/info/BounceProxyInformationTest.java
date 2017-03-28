package io.joynr.messaging.info;

/*
 * #%L
 * joynr::java::messaging::channel-service
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

import org.junit.Assert;
import org.junit.Test;

public class BounceProxyInformationTest {

    @Test
    public void testConstructorWithBpId() {

        BounceProxyInformation bpInfo = new BounceProxyInformation("bp1", URI.create("http://joyn.baseuri.io"));

        Assert.assertEquals("bp1", bpInfo.getId());
        Assert.assertEquals("http://joyn.baseuri.io", bpInfo.getLocation().toString());
    }

    @Test
    public void testEquals() {

        BounceProxyInformation bpInfo1 = new BounceProxyInformation("bp", URI.create("http://www.joyn.de"));
        BounceProxyInformation bpInfo2 = new BounceProxyInformation("bp", URI.create("http://www.joyn.de"));

        Assert.assertTrue(bpInfo1.equals(bpInfo2));
    }

    @Test
    public void testEqualsForSameReferences() {

        BounceProxyInformation bpInfo1 = new BounceProxyInformation("bp", URI.create("http://www.joyn.de"));

        Assert.assertTrue(bpInfo1.equals(bpInfo1));
    }

    @Test
    public void testNotEqualsForNull() {

        BounceProxyInformation bpInfo1 = new BounceProxyInformation("bp", URI.create("http://www.joyn.de"));

        Assert.assertFalse(bpInfo1.equals(null));
    }

    @Test
    public void testNotEqualDifferentIds() {

        BounceProxyInformation bpInfo1 = new BounceProxyInformation("bp1", URI.create("http://www.joyn.de"));
        BounceProxyInformation bpInfo2 = new BounceProxyInformation("bp2", URI.create("http://www.joyn.de"));

        Assert.assertFalse(bpInfo1.equals(bpInfo2));
    }

    @Test
    public void testNotEqualDifferentLocations() {

        BounceProxyInformation bpInfo1 = new BounceProxyInformation("bp", URI.create("http://www.joyn1.de"));
        BounceProxyInformation bpInfo2 = new BounceProxyInformation("bp", URI.create("http://www.joyn2.de"));

        Assert.assertFalse(bpInfo1.equals(bpInfo2));
    }
}
