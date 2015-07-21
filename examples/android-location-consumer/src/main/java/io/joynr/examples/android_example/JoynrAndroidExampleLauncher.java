package io.joynr.examples.android_example;

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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.joynrandroidruntime.JoynrAndroidRuntime;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import joynr.PeriodicSubscriptionQos;
import joynr.types.GpsLocation;
import joynr.vehicle.GpsProxy;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class JoynrAndroidExampleLauncher {
    private static final Logger logger = LoggerFactory.getLogger(JoynrAndroidExampleLauncher.class);
    private Output output; // Interface to a TextView to display log messages in the app
    private JoynrAndroidRuntime joynrAndroidRuntime; // Bound service for communication via joynr in background
    private static final String PROVIDER_DOMAIN = "android-domain";

    public void setJoynAndroidRuntime(JoynrAndroidRuntime joynrAndroidRuntime) {
        this.joynrAndroidRuntime = joynrAndroidRuntime;
    }

    public void createProxyAndGetLocation() {
        if (joynrAndroidRuntime == null) {
            logger.error("JoynrAndroidExampleLauncher has not been initialized!");
            logToOutput("JoynrAndroidExampleLauncher has not been initialized!\n");
        }
        logger.info("In JoynrAndroidExampleLauncher");
        logToOutput("Creating joynr GPS proxy and requesting location...\n");

        MessagingQos messagingQos = new MessagingQos(); // MessagingQos with defaul values. If messages are dropped due
        // to high latency, raise TTL values in MessagingQos.
        DiscoveryQos discoveryQos = new DiscoveryQos(30 * 1000, // 30 msec timeout to find a provider
                                                     ArbitrationStrategy.HighestPriority,
                                                     100); // maxAgeOfCachedProviders == 100 => Arbitration will
        // request a list of providers from the global
        // directory if no successful request has been made in
        // the last 100ms.

        try {
            ProxyBuilder<GpsProxy> builder = joynrAndroidRuntime.getProxyBuilder(PROVIDER_DOMAIN, GpsProxy.class);

            builder.setDiscoveryQos(discoveryQos)
                   .setMessagingQos(messagingQos)
                   .build(new ProxyCreatedCallback<GpsProxy>() {

                       @Override
                       public void onProxyCreated(GpsProxy proxy) {
                           logger.info("Proxy created");
                           try {
                               // single location request
                               GpsLocation location = proxy.getLocation();

                               // Subscription example
                               // subscribeToLocation(proxy);

                               logToOutput("location: " + location + "\n");
                           } catch (JoynrArbitrationException e) {
                               logToOutput("Arbitration failed!\n");
                           } catch (JoynrCommunicationException e) {
                               logToOutput(e.getMessage() + "\n");
                           } catch (JoynrRuntimeException e) {
                               logToOutput("Caught unhandled JoynrRuntimeException : " + e.getMessage() + "\n");
                           }

                       }

                       @Override
                       public void onProxyCreationError(String errorMessage) {
                           logToOutput("Error during proxy creation: " + errorMessage + "\n");

                       }
                   });

        } catch (Throwable e) {
            logToOutput("ERROR: create proxy failed: " + e.getMessage() + "\n");
            logger.error("create proxy failed: ", e);
        }

    }

    @SuppressWarnings("unused")
    private void subscribeToLocation(GpsProxy proxy) {

        long minInterval_ms = 1000; // defines how often an update may be sent
        long period_ms = 10000; // defines how long to wait before sending an update even if the value did not
        // change or when onChange is false
        long expiryDate_ms = System.currentTimeMillis() + 1 * 60 * 60 * 1000; // subscribe for one hour
        long alertInterval_ms = 20000; // defines how long to wait for an update before publicationMissed is called
        long publicationTtl_ms = 20000; // time to live for publication messages
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms,
                                                                      expiryDate_ms,
                                                                      alertInterval_ms,
                                                                      publicationTtl_ms);
        AttributeSubscriptionListener<GpsLocation> listener = new AttributeSubscriptionListener<GpsLocation>() {

            @Override
            public void onReceive(GpsLocation value) {
                // TODO handle incoming updates
                logToOutput("Received subscription update: " + value.toString());
            }

            @Override
            public void onError() {
                // TODO Handle missed updates

            }
        };
        proxy.subscribeToLocation(listener, subscriptionQos);
    }

    private void logToOutput(String string) {
        if (output != null) {
            output.append(string);
        }
    }

    public void setOutput(Output output) {
        this.output = output;
    }

}
