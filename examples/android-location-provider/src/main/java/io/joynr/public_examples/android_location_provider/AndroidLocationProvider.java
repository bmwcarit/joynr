package io.joynr.public_examples.android_location_provider;

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

import io.joynr.arbitration.ArbitrationConstants;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.public_examples.android_location_provider.MyLocation.LocationResult;

import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import joynr.types.CustomParameter;
import joynr.types.GpsFixEnum;
import joynr.types.GpsLocation;
import joynr.types.ProviderQos;
import joynr.vehicle.GpsAbstractProvider;
import android.content.Context;
import android.location.Location;

import com.google.common.collect.Lists;

public class AndroidLocationProvider extends GpsAbstractProvider {

    private Context applicationContext;

    private MyLocation myLocation;
    private LocationResult locationResult;
    private ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);

    private Output output;

    public AndroidLocationProvider(String keyword, Context applicationContext, Output output) {
        this.applicationContext = applicationContext;
        this.output = output;
        providerQos = new ProviderQos();
        List<CustomParameter> qosParameterList = Lists.newArrayList();
        qosParameterList.add(new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, keyword));
        providerQos.setCustomParameters(qosParameterList);
        providerQos.setPriority(System.currentTimeMillis());

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
                    joynrLocation.setGpsFix(GpsFixEnum.Mode3D);
                } else {
                    joynrLocation.setGpsFix(GpsFixEnum.ModeNoFix);
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
    public GpsLocation getLocation() {
        logToOutput("Method getLocation was called. Sending the location: " + location.toString());
        return location;
    }

    @Override
    public ProviderQos getProviderQos() {
        return providerQos;
    }

    @Override
    public void restartWithRetries(@JoynrRpcParam("gpsfix") Integer gpsfix) throws JoynrArbitrationException {
        // TODO Auto-generated method stub

    }

    @Override
    @JoynrRpcReturn(deserialisationType = IntegerToken.class)
    public Integer calculateAvailableSatellites() throws JoynrArbitrationException {
        // TODO Auto-generated method stub
        return null;
    }

    private void logToOutput(String string) {
        if (output != null) {
            output.append(string);
        }
    }
}
