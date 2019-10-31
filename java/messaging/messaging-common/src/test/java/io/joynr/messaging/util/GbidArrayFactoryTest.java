/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.util;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.GbidArrayFactory;

public class GbidArrayFactoryTest {

    @Test
    public void testEmptyGbid() {
        GbidArrayFactory subject = new GbidArrayFactory("");
        String[] result = subject.create();
        assertEquals(1, result.length);
        assertEquals(result[0], "");
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testEmptyAtBeginningGbid() {
        String gbids = ",gbid3";
        new GbidArrayFactory(gbids);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testEmptyGbidInTheMiddle() {
        new GbidArrayFactory("gbid1,,gbid3");
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testOnlyCommaInGbidString() {
        new GbidArrayFactory(",");
    }

    @Test
    public void testSuccessfulGbidArrayCreation() {
        String gbids = " gbid1, gbid2 , gbid3 ";
        String[] gbidArray = gbids.replaceAll("\\s", "").split(",");
        GbidArrayFactory subject = new GbidArrayFactory(gbids);
        String[] result = subject.create();
        assertEquals(3, result.length);
        for (int i = 0; i < gbidArray.length; i++) {
            assertEquals(result[i], gbidArray[i]);
        }
    }
}
