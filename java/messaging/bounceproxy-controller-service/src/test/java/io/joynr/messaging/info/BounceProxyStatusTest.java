package io.joynr.messaging.info;

/*
 * #%L
 * joynr::java::messaging::bounceproxy-controller-service
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

import static io.joynr.messaging.info.BounceProxyStatus.ACTIVE;
import static io.joynr.messaging.info.BounceProxyStatus.ALIVE;
import static io.joynr.messaging.info.BounceProxyStatus.EXCLUDED;
import static io.joynr.messaging.info.BounceProxyStatus.SHUTDOWN;
import static io.joynr.messaging.info.BounceProxyStatus.UNRESOLVED;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class BounceProxyStatusTest {

    @Test
    public void checkTransitionsForAlive() {

        assertTrue(ALIVE.isValidTransition(ALIVE));
        assertTrue(ALIVE.isValidTransition(ACTIVE));
        assertTrue(ALIVE.isValidTransition(EXCLUDED));
        assertTrue(ALIVE.isValidTransition(SHUTDOWN));
        assertTrue(ALIVE.isValidTransition(UNRESOLVED));
    }

    @Test
    public void checkTransitionsForActive() {

        assertTrue(ACTIVE.isValidTransition(ALIVE));
        assertTrue(ACTIVE.isValidTransition(ACTIVE));
        assertTrue(ACTIVE.isValidTransition(EXCLUDED));
        assertTrue(ACTIVE.isValidTransition(SHUTDOWN));
        assertTrue(ACTIVE.isValidTransition(UNRESOLVED));
    }

    @Test
    public void checkTransitionsForExcluded() {

        assertFalse(EXCLUDED.isValidTransition(ALIVE));
        assertFalse(EXCLUDED.isValidTransition(ACTIVE));
        assertTrue(EXCLUDED.isValidTransition(EXCLUDED));
        assertFalse(EXCLUDED.isValidTransition(SHUTDOWN));
        assertFalse(EXCLUDED.isValidTransition(UNRESOLVED));
    }

    @Test
    public void checkTransitionsForShutdown() {

        assertTrue(SHUTDOWN.isValidTransition(ALIVE));
        assertFalse(SHUTDOWN.isValidTransition(ACTIVE));
        assertFalse(SHUTDOWN.isValidTransition(EXCLUDED));
        assertTrue(SHUTDOWN.isValidTransition(SHUTDOWN));
        assertFalse(SHUTDOWN.isValidTransition(UNRESOLVED));
    }

    @Test
    public void checkTransitionsForUnresolved() {

        assertFalse(UNRESOLVED.isValidTransition(ALIVE));
        assertFalse(UNRESOLVED.isValidTransition(ACTIVE));
        assertFalse(UNRESOLVED.isValidTransition(EXCLUDED));
        assertFalse(UNRESOLVED.isValidTransition(SHUTDOWN));
        assertTrue(UNRESOLVED.isValidTransition(UNRESOLVED));
    }
}
