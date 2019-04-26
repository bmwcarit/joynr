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

import android.os.AsyncTask;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import joynr.PeriodicSubscriptionQos;
import joynr.types.Localisation.GpsLocation;
import joynr.vehicle.GpsProxy;

class CreateSubscriptionTask extends AsyncTask<GpsProxy, Void, Void> {
    private static final Logger logger = LoggerFactory.getLogger(CreateSubscriptionTask.class);
    private final Output output;

    private Exception exception;

    public CreateSubscriptionTask(Output output) {
        this.output = output;
    }

    @Override
    protected Void doInBackground(GpsProxy... params) {
        try {
            subscribeToLocation(params[0]);
        } catch (Exception e) {
            this.exception = e;
        }
        return null;
    }

    @Override
    protected void onPostExecute(Void x) {
        if (exception != null) {
            logToOutput("unable to subscribe to Gps Location: " + exception.getMessage());
            logger.debug("unable to subscribe to Gps Location", exception);
        }
    }

    private void subscribeToLocation(GpsProxy proxy) {

        long minInterval_ms = 1000; // defines how often an update may be sent
        long periodMs = 10000; // defines how long to wait before sending an update even if the value did not
        // change or when onChange is false
        long validityMs = 1 * 60 * 60 * 1000; // subscribe for one hour
        long alertIntervalMs = 20000; // defines how long to wait for an update before publicationMissed is called
        long publicationTtlMs = 20000; // time to live for publication messages
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos().setPeriodMs(periodMs)
                                                                       .setValidityMs(validityMs)
                                                                       .setAlertAfterIntervalMs(alertIntervalMs)
                                                                       .setPublicationTtlMs(publicationTtlMs);
        AttributeSubscriptionAdapter<GpsLocation> listener = new AttributeSubscriptionAdapter<GpsLocation>() {

            @Override
            public void onReceive(GpsLocation value) {
                logToOutput("Received subscription update: " + value.toString());
            }

            @Override
            public void onError(JoynrRuntimeException error) {
                logToOutput("error in subscription: " + error.getMessage());

            }
        };
        proxy.subscribeToLocation(listener, subscriptionQos);
    }

    private void logToOutput(String string) {
        if (output != null) {
            output.append(string);
        }
    }
}
