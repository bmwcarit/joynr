package io.joynr.dispatching.subscription;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import io.joynr.pubsub.publication.AttributeListener;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import joynr.exceptions.ProviderRuntimeException;
import joynr.tests.DefaulttestProvider;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;

public class SubscriptionTestsProviderImpl extends DefaulttestProvider {

    public static final String MESSAGE_PROVIDERRUNTIMEEXCEPTION = "ProviderRuntimeException";
    public static final String MESSAGE_THROWN_PROVIDERRUNTIMEEXCEPTION = "thrownException";

    List<Integer> list = new ArrayList<Integer>();

    public SubscriptionTestsProviderImpl() {
        testAttribute = 42;
        complexTestAttribute = new GpsLocation();
        complexTestAttribute.setLatitude(48.143554);
        complexTestAttribute.setLongitude(11.536564);
        complexTestAttribute.setAltitude(6.0);
        complexTestAttribute.setGpsFix(GpsFixEnum.MODE3D);
        ATTRIBUTEWITHCAPITALLETTERS = 42;
    }

    @Override
    public Promise<Deferred<Integer>> getTestAttribute() {
        Deferred<Integer> deferred = new Deferred<Integer>();
        deferred.resolve(testAttribute++);
        return new Promise<Deferred<Integer>>(deferred);
    }

    @Override
    public Promise<Deferred<Integer[]>> getListOfInts() {
        Deferred<Integer[]> deferred = new Deferred<Integer[]>();
        list.add(testAttribute++);
        deferred.resolve(list.toArray(new Integer[list.size()]));
        return new Promise<Deferred<Integer[]>>(deferred);
    }

    public GpsLocation getComplexTestAttributeSync() {
        return complexTestAttribute;
    }

    @Override
    public Promise<Deferred<Integer>> getAttributeWithProviderRuntimeException() {
        Deferred<Integer> deferred = new Deferred<Integer>();
        ProviderRuntimeException error = new ProviderRuntimeException(MESSAGE_PROVIDERRUNTIMEEXCEPTION);
        deferred.reject(error);
        return new Promise<Deferred<Integer>>(deferred);
    }

    @Override
    public Promise<Deferred<Integer>> getAttributeWithThrownException() {
        throw new IllegalArgumentException(MESSAGE_THROWN_PROVIDERRUNTIMEEXCEPTION);
    }

    Set<String> attributeSubscriptionArrived = new HashSet<>();

    public void waitForAttributeSubscription(String attributeName) {
        synchronized (this) {
            while (!attributeSubscriptionArrived.contains(attributeName)) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                }
            }
        }
    }

    public void waitForAttributeUnsubscription(String attributeName) {
        synchronized (this) {
            while (attributeSubscriptionArrived.contains(attributeName)) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                }
            }
        }
    }

    @Override
    public void registerAttributeListener(String attributeName,
            AttributeListener attributeListener) {
        super.registerAttributeListener(attributeName, attributeListener);
        synchronized (this) {
            if (!attributeSubscriptionArrived.contains(attributeName)) {
                attributeSubscriptionArrived.add(attributeName);
                this.notify();
            }
        }
    }

    @Override
    public void unregisterAttributeListener(String attributeName,
            AttributeListener attributeListener) {
        super.unregisterAttributeListener(attributeName, attributeListener);
        synchronized (this) {
            if (attributeSubscriptionArrived.contains(attributeName)) {
                attributeSubscriptionArrived.remove(attributeName);
                this.notify();
            }
        }
    }
}
