/**
 *
 */
package io.joynr.caching;

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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import io.joynr.qos.QualityOfService;
import io.joynr.util.GuiceBasedTest;

import java.util.NoSuchElementException;

import org.junit.Test;

import com.google.common.collect.Lists;
import com.google.inject.Binder;
import com.google.inject.Inject;
import com.google.inject.Module;

public class ClientCacheTest extends GuiceBasedTest {

    @Inject
    ClientCache fixture;

    private String attributeId = "door.front.right.open";

    @Override
    public Iterable<Module> getModules() {
        return Lists.<Module> newArrayList(new Module() {
            @Override
            public void configure(Binder binder) {
                binder.bind(ClientCache.class).to(ClientHashMapCache.class);
            }
        });
    }

    @Test(expected = NoSuchElementException.class)
    public void lookupThrowsExceptionWhenAttributeDoesNotExist() throws Exception {
        fixture.lookUp("aaa");
    }

    @Test
    public void lookupReturnsCachedValue() throws Exception {
        Object value = Boolean.TRUE;
        fixture.insert(attributeId, value);
        assertEquals(value, fixture.lookUp(attributeId));
    }

    @Test
    public void insertOverwritesPreviousEntryCorrectly() {
        Object value = Boolean.TRUE;
        fixture.insert(attributeId, value);
        value = Boolean.FALSE;
        fixture.insert(attributeId, value);
        assertFalse((Boolean) fixture.lookUp(attributeId));
    }

    @Test(expected = NoSuchElementException.class)
    public void testCacheCleansOldValues() {

        QualityOfService qos = new QualityOfService();
        qos.setCacheTimeToLiveMs(500);
        fixture.setQoS(qos);
        fixture.insert(attributeId, Boolean.TRUE);

        waitOneSecond();

        fixture.cleanUp();
        fixture.lookUp(attributeId);
    }

    @Test
    public void testCacheDoesNotCleanNewValues() {
        QualityOfService qos = new QualityOfService();
        qos.setCacheTimeToLiveMs(500);
        fixture.setQoS(qos);
        fixture.insert(attributeId, Boolean.TRUE);
        fixture.cleanUp();
        assertEquals(Boolean.TRUE, fixture.lookUp(attributeId));
    }

    @Test
    public void cacheValueShouldBeValidWhenQoSMeetsDataFreshnessConstraints() throws Exception {
        QualityOfService qos = new QualityOfService();
        qos.setDataFreshnessMs(2000);
        fixture.setQoS(qos);
        fixture.insert(attributeId, Boolean.TRUE);
        assertTrue(fixture.isCacheValueValid(attributeId));
    }

    @Test
    public void cacheValueIsNotValidWhenQoSDataFreshnessExpires() throws Exception {
        QualityOfService qos = new QualityOfService();
        qos.setDataFreshnessMs(900);
        fixture.setQoS(qos);
        fixture.insert(attributeId, Boolean.TRUE);
        waitOneSecond();
        assertFalse(fixture.isCacheValueValid(attributeId));
    }

    private void waitOneSecond() {
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
        }
    }
}
