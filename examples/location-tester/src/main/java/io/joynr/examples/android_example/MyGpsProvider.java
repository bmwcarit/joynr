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
package io.joynr.examples.android_example;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;
import joynr.vehicle.DefaultGpsProvider;

public class MyGpsProvider extends DefaultGpsProvider {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final Logger logger = LoggerFactory.getLogger(MyGpsProvider.class);

    private int time = 0;

    public MyGpsProvider() {
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
    public Promise<Deferred<GpsLocation>> getLocation() {
        Deferred<GpsLocation> deferred = new Deferred<GpsLocation>();
        location.setTime(time++);
        logger.info(PRINT_BORDER + "getLocation -> " + location + PRINT_BORDER);
        deferred.resolve(location);
        return new Promise<Deferred<GpsLocation>>(deferred);
    }

    public void notifyLocationUpdate() {
        location.setTime(time++);
        logger.info(PRINT_BORDER + "notify location update: " + location + PRINT_BORDER);
        locationChanged(location);
    }
}
