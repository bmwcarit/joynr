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

import java.util.Properties;
import java.util.Scanner;
import java.util.regex.Pattern;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.types.Localisation.GpsLocation;
import joynr.vehicle.GpsProxy;

public class GpsConsumerApplication extends AbstractJoynrApplication {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final Logger logger = LoggerFactory.getLogger(GpsConsumerApplication.class);
    public static final String APP_CONFIG_PROVIDER_DOMAIN = "javademoapp.provider.domain";
    private static final String STATIC_PERSISTENCE_FILE = "gps-consumer-joynr.properties";

    @Inject
    @Named(APP_CONFIG_PROVIDER_DOMAIN)
    private String providerDomain;
    private GpsProxy gpsProxy;
    private Future<String> subscriptionFuture;

    /**
     * Main method. This method is responsible for: 1. Instantiating the consumer application. 2. Injecting the instance
     * with Guice bindings 3. Starting the application. 4. Ending the application so that the necessary clean up calls
     * are made.
     *
     * @param args arguments give when calling the main method
     */
    public static void main(String[] args) {
        // run application from cmd line using Maven:
        // mvn exec:java -Dexec.classpathScope="test" -Dexec.mainClass="io.joynr.public_examples.android_location_provider.GpsConsumerApplication" -Dexec.args="<provider-domain>"
        if (args.length != 1) {
            logger.error("USAGE: java {} <provider-domain>", GpsConsumerApplication.class.getName());
            return;
        }
        String providerDomain = args[0];
        logger.debug("Searching for providers on domain \"{}\"", providerDomain);

        // joynr config properties are used to set joynr configuration at compile time. They are set on the
        // JoynInjectorFactory.
        Properties joynrConfig = new Properties();
        // Set a custom static persistence file (default is joynr.properties in the working dir) to store
        // joynr configuration. It allows for changing the joynr configuration at runtime. Custom persistence
        // files support running the consumer and provider applications from within the same directory.
        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);

        // How to use custom infrastructure elements:

        // 1) Set them programmatically at compile time using the joynr configuration properties at the
        // JoynInjectorFactory. E.g. uncomment the following lines to set a certain joynr server
        // instance.
        // joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS, "joynrdefaultgbid");
        // joynrConfig.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:1883/");
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, "test_consumer_local_domain");

        // NOTE: When running this application to test the android-location-provider, you must use
        //       the concrete hostname (and _not_ localhost) in the bounceproxy URL, since this URL
        //       is registered in the global discovery directory and must be resolvable by the Android
        //       device.
        // joynrConfig.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, "http://<concrete host>:8080/bounceproxy/");

        // 2) Or set them in the static persistence file (default: joynr.properties in working dir) at
        // runtime. If not available in the working dir, it will be created during the first launch
        // of the application. Copy the following lines to the custom persistence file to set a
        // certain joynr server instance.
        // NOTE: This application uses a custom static persistence file consumer-joynr.properties.
        // Copy the following lines to the custom persistence file to set a certain joynr server
        // instance.
        // joynr.messaging.gbids=joynrdefaultgbid
        // joynr.messaging.mqtt.brokeruris=tcp://localhost:1883/

        // 3) Or set them in Java System properties.
        // -Djoynr.messaging.gbids=joynrdefaultgbid
        // -Djoynr.messaging.mqtt.brokeruris=tcp://localhost:1883/

        // NOTE:
        // Programmatically set configuration properties override properties set in the static persistence file.
        // Java system properties override both

        // Application-specific configuration properties are injected to the application by setting
        // them on the JoynApplicationModule.
        Properties appConfig = new Properties();
        appConfig.setProperty(APP_CONFIG_PROVIDER_DOMAIN, providerDomain);

        JoynrApplication gpsConsumerApp = new JoynrInjectorFactory(joynrConfig,
                                                                   Modules.override(new CCInProcessRuntimeModule())
                                                                          .with(new AtmosphereMessagingModule())).createApplication(new JoynrApplicationModule(GpsConsumerApplication.class,
                                                                                                                                                               appConfig));
        gpsConsumerApp.run();

        pressQEnterToContinue();

        gpsConsumerApp.shutdown();
    }

    @Override
    public void shutdown() {
        if (gpsProxy != null) {
            if (subscriptionFuture != null) {
                try {
                    String subscriptionIdLocation = subscriptionFuture.get();
                    gpsProxy.unsubscribeFromLocation(subscriptionIdLocation);
                } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                    logger.error(e.toString());
                }
            }
        }

        // Add any clean up code here for your application.
        runtime.shutdown(true);

        // TODO currently there is a bug preventing all threads being stopped
        // WORKAROUND
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            // do nothing; exiting application
        }
        System.exit(0);
    }

    @Override
    public void run() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        // As soon as the arbitration QoS is set on the proxy builder, discovery of suitable providers
        // is triggered. If the discovery process does not find matching providers within the
        // arbitration timeout duration it will be terminated and you will get an arbitration exception.
        discoveryQos.setDiscoveryTimeoutMs(10000);
        // Provider entries in the global capabilities directory are cached locally. Discovery will
        // consider entries in this cache valid if they are younger as the max age of cached
        // providers as defined in the QoS. All valid entries will be processed by the arbitrator when searching
        // for and arbitrating the "best" matching provider.
        // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
        // directory. Therefore, not all providers registered with the global capabilities
        // directory might be taken into account during arbitration.
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        // The discovery process outputs a list of matching providers. The arbitration strategy then
        // chooses one or more of them to be used by the proxy.
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        // The provider will maintain at least a minimum interval idle time in milliseconds between
        // successive notifications, even if on-change notifications are enabled and the value changes more
        // often. This prevents the consumer from being flooded by updated values. The filtering happens on
        // the provider's side, thus also preventing excessive network traffic.
        int minInterval_ms = 0;
        // The provider will send notifications every maximum interval in milliseconds, even if the value didn't
        // change. It will send notifications more often if on-change notifications are enabled,
        // the value changes more often, and the minimum interval QoS does not prevent it. The maximum interval
        // can thus be seen as a sort of heart beat.
        int maxInterval_ms = 10000;

        // The provider will send notifications until the end date is reached. The consumer will not receive any
        // notifications (neither value notifications nor missed publication notifications) after
        // this date.
        long validity_ms = 60000;
        // If no notification was received within the last alert interval, a missed publication
        // notification will be raised.
        int alertAfterInterval_ms = 20000;
        // Notification messages will be sent with this time-to-live. If a notification message can not be
        // delivered within its TTL, it will be deleted from the system.
        // NOTE: If a notification message is not delivered due to an expired TTL, it might raise a
        // missed publication notification (depending on the value of the alert interval QoS).
        int publicationTtl_ms = 5000;

        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
        subscriptionQos.setMinIntervalMs(minInterval_ms).setMaxIntervalMs(maxInterval_ms).setValidityMs(validity_ms);
        subscriptionQos.setAlertAfterIntervalMs(alertAfterInterval_ms).setPublicationTtlMs(publicationTtl_ms);

        ProxyBuilder<GpsProxy> proxyBuilder = runtime.getProxyBuilder(providerDomain, GpsProxy.class);

        // reading an attribute value
        gpsProxy = proxyBuilder.setMessagingQos(new MessagingQos()).setDiscoveryQos(discoveryQos).build();

        subscriptionFuture = gpsProxy.subscribeToLocation(new AttributeSubscriptionAdapter<GpsLocation>() {

            @Override
            public void onReceive(GpsLocation value) {
                logger.info(PRINT_BORDER + "SUBSCRIPTION: location: " + value + PRINT_BORDER);
            }

            @Override
            public void onError(JoynrRuntimeException error) {
                logger.info(PRINT_BORDER + "SUBSCRIPTION: location, publication missed " + PRINT_BORDER);
            }
        }, subscriptionQos);
    }

    static void pressQEnterToContinue() {
        try {
            // sleep a while to have the log output at the end
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // nothing to do
        }
        logger.info("\n\n\n************************************************\n Please press \"q + <Enter>\" to quit application\n************************************************\n\n");
        Scanner input = new Scanner(System.in);
        Pattern pattern = Pattern.compile("q");
        // wait until the user types q to quit
        input.next(pattern);
        input.close();
    }

}
