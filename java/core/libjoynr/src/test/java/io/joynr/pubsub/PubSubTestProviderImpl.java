package io.joynr.pubsub;

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

import java.util.ArrayList;
import java.util.List;

import joynr.tests.DefaultTestProvider;
import joynr.types.GpsFixEnum;
import joynr.types.GpsLocation;

public class PubSubTestProviderImpl extends DefaultTestProvider {

    List<Integer> list = new ArrayList<Integer>();

    public PubSubTestProviderImpl() {
        testAttribute = 42;
        complexTestAttribute = new GpsLocation();
        complexTestAttribute.setLatitude(48.143554);
        complexTestAttribute.setLongitude(11.536564);
        complexTestAttribute.setAltitude(6.0);
        complexTestAttribute.setGpsFix(GpsFixEnum.MODE3D);
    }

    @Override
    public Integer getTestAttribute() {
        return testAttribute++;
    }

    @Override
    public List<Integer> getListOfInts() {
        list.add(testAttribute++);
        return list;
    }

}
