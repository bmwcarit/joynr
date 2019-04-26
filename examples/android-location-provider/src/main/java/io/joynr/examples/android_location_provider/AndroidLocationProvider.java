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
package io.joynr.examples.android_location_provider;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.location.Location;

import io.joynr.examples.android_location_provider.MyLocation.LocationResult;
import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;
import joynr.vehicle.DefaultGpsProvider;

public class AndroidLocationProvider extends DefaultGpsProvider {

    private Context applicationContext;

    private MyLocation myLocation;
    private LocationResult locationResult;
    private ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);

    private Output output;

    public AndroidLocationProvider(Context applicationContext, Output output) {
        this.applicationContext = applicationContext;
        this.output = output;
        this.location = new GpsLocation();
        initLocationProviderAndListener();
    }

    private void initLocationProviderAndListener() {

        // LocationResult.gotLocation will be called when android notifies about a new location
        locationResult = new LocationResult() {
            @Override
            public void gotLocation(Location androidLocation) {
                GpsLocation joynrLocation = new GpsLocation();
                if (androidLocation != null) {
                    joynrLocation.setLatitude(androidLocation.getLatitude());
                    joynrLocation.setLongitude(androidLocation.getLongitude());
                    joynrLocation.setAltitude(androidLocation.getAltitude());
                    joynrLocation.setGpsFix(GpsFixEnum.MODE3D);
                } else {
                    joynrLocation.setGpsFix(GpsFixEnum.MODENOFIX);
                }
                // applies the location to the location provider and notifies onChange subscriptions
                locationChanged(joynrLocation);
            }
        };

        // Use myLocation to asynchronously obtain a location
        myLocation = new MyLocation();
        myLocation.getLocation(applicationContext, locationResult);

        // Attempt to update the location every 20 seconds
        scheduler.scheduleWithFixedDelay(new Runnable() {

            @Override
            public void run() {
                myLocation.getLocation(applicationContext, locationResult);
            }
        }, 0, 20000, TimeUnit.MILLISECONDS);

    }

    @Override
    public Promise<Deferred<GpsLocation>> getLocation() {
        Deferred<GpsLocation> deferred = new Deferred<GpsLocation>();
        logToOutput("Method getLocation was called. Sending the location: " + location.toString());
        deferred.resolve(location);
        return new Promise<Deferred<GpsLocation>>(deferred);
    }

    private void logToOutput(String string) {
        if (output != null) {
            output.append(string);
        }
    }
}
