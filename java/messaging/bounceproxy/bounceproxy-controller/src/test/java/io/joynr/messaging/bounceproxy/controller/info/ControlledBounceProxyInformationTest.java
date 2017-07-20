package io.joynr.messaging.bounceproxy.controller.info;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.net.URI;

import org.junit.Assert;
import org.junit.Test;

public class ControlledBounceProxyInformationTest {

    @Test
    public void testConstructorWithClusterAndInstance() {

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("cluster0.instance0",
                                                                                       URI.create("http://joyn.baseuri.io"));

        Assert.assertEquals("cluster0.instance0", bpInfo.getId());
        Assert.assertEquals("instance0", bpInfo.getInstanceId());
        Assert.assertEquals("http://joyn.baseuri.io", bpInfo.getLocation().toString());
    }

    @Test
    public void testConstructorWithBpId() {

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("cluster0.instance0",
                                                                                       URI.create("http://joyn.baseuri.io"));

        Assert.assertEquals("cluster0.instance0", bpInfo.getId());
        Assert.assertEquals("instance0", bpInfo.getInstanceId());
        Assert.assertEquals("http://joyn.baseuri.io", bpInfo.getLocation().toString());
    }

    @Test
    public void testConstructorWithBpIdInWrongFormat() {

        try {
            new ControlledBounceProxyInformation("cluster0-instance0", URI.create("http://joyn.baseuri.io"));
            Assert.fail("Should fail with IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testEquals() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        Assert.assertTrue(bpInfo1.equals(bpInfo2));
    }

    @Test
    public void testEqualsForSameReferences() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        Assert.assertTrue(bpInfo1.equals(bpInfo1));
    }

    @Test
    public void testNotEqualsForNull() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        Assert.assertFalse(bpInfo1.equals(null));
    }

    @Test
    public void testNotEqualsDifferentInstanceId() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("A",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        Assert.assertFalse(bpInfo1.equals(bpInfo2));
    }

    @Test
    public void testNotEqualsDifferentClusterId() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("X",
                                                                                        "B",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        Assert.assertFalse(bpInfo1.equals(bpInfo2));
    }

    @Test
    public void testNotEqualsDifferentUrlForCc() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynA.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        Assert.assertFalse(bpInfo1.equals(bpInfo2));
    }

    @Test
    public void testNotEqualsDifferentUrlForBpc() {

        ControlledBounceProxyInformation bpInfo1 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynX.bmwgroup.net"));

        ControlledBounceProxyInformation bpInfo2 = new ControlledBounceProxyInformation("X",
                                                                                        "Y",
                                                                                        URI.create("http://www.joynX.de"),
                                                                                        URI.create("http://joynA.bmwgroup.net"));

        Assert.assertFalse(bpInfo1.equals(bpInfo2));
    }
}
