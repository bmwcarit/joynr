package io.joynr.messaging;

/*
 * #%L
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

import org.junit.Test;

/**
 * Unit tests for the {@link MessagingQos}.
 */
public class MessagingQosTest {

    @Test
    public void testDefaultEffort() {
        MessagingQos messagingQos = new MessagingQos(0L);
        assertEquals(MessagingQosEffort.NORMAL, messagingQos.getEffort());
    }

    @Test
    public void testCustomEffort() {
        MessagingQos messagingQos = new MessagingQos(0L, MessagingQosEffort.BEST_EFFORT);
        assertEquals(MessagingQosEffort.BEST_EFFORT, messagingQos.getEffort());
    }

    @Test
    public void testEffortSetter() {
        MessagingQos messagingQos = new MessagingQos(0L);
        assertEquals(MessagingQosEffort.NORMAL, messagingQos.getEffort());
        messagingQos.setEffort(MessagingQosEffort.BEST_EFFORT);
        assertEquals(MessagingQosEffort.BEST_EFFORT, messagingQos.getEffort());
    }

    @Test
    public void testDefaultEncrypt() {
        MessagingQos messagingQos = new MessagingQos(0L);
        assertEquals(false, messagingQos.getEncrypt());
    }

    @Test
    public void testCustomEncrypt() {
        boolean encrypt = true;
        MessagingQos messagingQos = new MessagingQos(0L, MessagingQosEffort.BEST_EFFORT, encrypt);
        assertEquals(encrypt, messagingQos.getEncrypt());

        messagingQos = new MessagingQos(0L, encrypt);
        assertEquals(encrypt, messagingQos.getEncrypt());
    }

    @Test
    public void testEncryptSetter() {
        MessagingQos messagingQos = new MessagingQos(0L);
        assertEquals(false, messagingQos.getEncrypt());
        boolean encrypt = true;
        messagingQos.setEncrypt(encrypt);
        assertEquals(encrypt, messagingQos.getEncrypt());
    }

    @Test
    public void testDefaultCompress() {
        MessagingQos messagingQos = new MessagingQos(0L);
        assertEquals(false, messagingQos.getCompress());
    }

    @Test
    public void testCompressSetter() {
        MessagingQos messagingQos = new MessagingQos(0L);
        assertEquals(false, messagingQos.getCompress());
        boolean compress = true;
        messagingQos.setCompress(compress);
        assertEquals(compress, messagingQos.getCompress());
    }
}
