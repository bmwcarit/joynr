package io.joynr.demo;

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
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.pubsub.subscription.SubscriptionListener;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.util.Properties;

import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.types.GpsLocation;
import joynr.vehicle.GpsProxy;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class GpsConsumerApplication extends AbstractJoynrApplication {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final String INJECT_PROVIDER_DOMAIN = "JavaDemoApp.MyRadioProvider.domain";
    private static final Logger LOG = LoggerFactory.getLogger(GpsConsumerApplication.class);
    private static final String STATIC_PERSISTANCE_FILE = "consumer-joynr.properties";
    private static JoynrApplication gpsConsumerApp;

    @Inject
    @Named(INJECT_PROVIDER_DOMAIN)
    private String providerDomain;

    /**
     * Main method. This method is responsible for: 1. Instantiating the consumer application. 2. Injecting the instance
     * with Guice bindings. 3. Starting the application. 4. Ending the application so that the necessary clean up calls
     * are made.
     */
    public static void main(String[] args)  {
        // set the provider domain here
        // NOTE: if the provider is started first, the provider domain will be loaded from properties file.
        //       This only works if provider and consumer are executed from the same working directory.
        String providerDomain = "android_location";

        Properties joynrConfig = new Properties();

        // ***************
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, "myTestDomain"); // <-- Set your domain here !!!!!!!!!!!!!
        // ***************

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTANCE_FILE);

        Properties config = new Properties();
        config.setProperty(INJECT_PROVIDER_DOMAIN, providerDomain);

        gpsConsumerApp = new JoynrInjectorFactory(joynrConfig).createApplication(new JoynrApplicationModule(GpsConsumerApplication.class,
                                                                                                         config));
        gpsConsumerApp.run();
        MyRadioHelper.pressQEnterToContinue();
        gpsConsumerApp.shutdown();
    }

    @Override
    public void shutdown() {
        // Add any clean up code here for your application.
        runtime.shutdown(true);
    }

    @Override
    public void run() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        // As soon as the arbitration QoS is set on the proxy builder, discovery of suitable providers
        // is triggered. If the discovery process does not find matching providers within the 
        // arbitration timeout duration it will be terminated and you will get an arbitration exception.
        discoveryQos.setDiscoveryTimeout(10000);
        // Provider entries in the global capabilities directory are cached locally. Discovery will
        // consider entries in this cache valid if they are younger as the max age of cached 
        // providers as defined in the QoS. All valid entries will be processed by the arbitrator when searching
        // for and arbitrating the "best" matching provider.
        // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
        //       directory. Therefore, not all providers registered with the global capabilities
        //       directory might be taken into account during arbitration.
        discoveryQos.setCacheMaxAge(Long.MAX_VALUE);
        // The discovery process outputs a list of matching providers. The arbitration strategy then 
        // chooses one or more of them to be used by the proxy.
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        // The provider will maintain at least a minimum interval idle time in milliseconds between 
        // successive notifications, even if on-change notifications are enabled and the value changes more
        // often. This prevents the consumer from being flooded by updated values. The filtering happens on
        // the provider's side, thus also preventing excessive network traffic.
        int minInterval_ms = 1000;
        // The provider will send notifications every maximum interval in milliseconds, even if the value didn't
        // change. It will send notifications more often if on-change notifications are enabled,
        // the value changes more often, and the minimum interval QoS does not prevent it. The maximum interval 
        // can thus be seen as a sort of heart beat.
        int maxInterval_ms = 30000;

        // The provider will send notifications until the end date is reached. The consumer will not receive any
        // notifications (neither value notifications nor missed publication notifications) after
        // this date.
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        // If no notification was received within the last alert interval, a missed publication
        // notification will be raised. 
        int alertAfterInterval_ms = 40000;
        // Notification messages will be sent with this time-to-live. If a notification message can not be
        // delivered within its TTL, it will be deleted from the system.
        // NOTE: If a notification message is not delivered due to an expired TTL, it might raise a
        //       missed publication notification (depending on the value of the alert interval QoS).
        int publicationTtl_ms = 5000;

        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms, maxInterval_ms, expiryDate_ms, alertAfterInterval_ms, publicationTtl_ms);

        ProxyBuilder<GpsProxy> proxyBuilder = runtime.getProxyBuilder(providerDomain, GpsProxy.class);

        try {

            // create a proxy for the interface you want to query
            GpsProxy proxy = proxyBuilder.setMessagingQos(new MessagingQos()).setDiscoveryQos(discoveryQos).build();

            proxy.subscribeToLocation(new SubscriptionListener<GpsLocation>() {

                @Override
                public void receive(GpsLocation value) {
                    LOG.info(PRINT_BORDER + "SUBSCRIPTION gps location: " + value + PRINT_BORDER);
                }

                @Override
                public void publicationMissed() {
                    LOG.info(PRINT_BORDER + "SUBSCRIPTION: publication missed " + PRINT_BORDER);
                }
            }, subscriptionQos);
        } catch (JoynrArbitrationException e) {
            LOG.error("No provider found", e);
        } catch (JoynrIllegalStateException e) {
            LOG.error("The API was used incorrectly", e);
        } catch (JoynrCommunicationException e) {
            LOG.error("The message was not sent: ", e);
        } catch (InterruptedException e) {
            LOG.error("The message was interupted: ", e);
        }
    }
}
