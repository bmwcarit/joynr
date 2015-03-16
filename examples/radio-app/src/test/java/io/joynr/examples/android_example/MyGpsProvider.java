package io.joynr.examples.android_example;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.exceptions.JoynrArbitrationException;
import joynr.types.GpsFixEnum;
import joynr.types.GpsLocation;
import joynr.vehicle.GpsAbstractProvider;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MyGpsProvider extends GpsAbstractProvider {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final Logger LOG = LoggerFactory.getLogger(MyGpsProvider.class);

    private GpsLocation location;
    private int time = 0;

    public MyGpsProvider() {
        providerQos.setPriority(System.currentTimeMillis());
        location = new GpsLocation(11.5819810, // longitude
                                   48.1351250, // latitude
                                   532.0, // altitude,
                                   GpsFixEnum.MODE3D, // gpsFix
                                   0.0, // heading
                                   0.0, // quality
                                   0.0, // elevation
                                   0.0, // bearing
                                   0L, // gpsTime
                                   0L, // deviceTime
                                   time // time
        );
    }

    @Override
    public void restartWithRetries(Integer gpsfix) throws JoynrArbitrationException {
        // TODO Auto-generated method stub

    }

    @Override
    public Integer calculateAvailableSatellites() throws JoynrArbitrationException {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public GpsLocation getLocation() {
        location.setTime(time++);
        LOG.info(PRINT_BORDER + "getLocation -> " + location + PRINT_BORDER);
        return location;
    }

    public void notifyLocationUpdate() {
        location.setTime(time++);
        LOG.info(PRINT_BORDER + "notify location update: " + location + PRINT_BORDER);
        locationChanged(location);
    }
}
