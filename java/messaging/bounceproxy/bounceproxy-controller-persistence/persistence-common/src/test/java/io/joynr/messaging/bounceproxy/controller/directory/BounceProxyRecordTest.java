package io.joynr.messaging.bounceproxy.controller.directory;

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

import static org.junit.Assert.assertEquals;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.net.URI;

import org.junit.Before;
import org.junit.Test;

public class BounceProxyRecordTest {

    @Before
    public void setUp() throws Exception {
    }

    @Test
    public void testCreation() {

        ControlledBounceProxyInformation info = new ControlledBounceProxyInformation("X.Y",
                                                                                     URI.create("http://www.testuri.com/test"));
        BounceProxyRecord record = new BounceProxyRecord(info);

        assertEquals(info, record.getInfo());
        assertEquals(0, record.getNumberOfAssignedChannels());
        assertEquals(BounceProxyRecord.ASSIGNMENT_TIMESTAMP_NEVER, record.getLastAssignedTimestamp());
        assertEquals(BounceProxyStatus.ALIVE, record.getStatus());
    }

}
