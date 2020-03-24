package io.joynr.androidhelloworldprovider;

import java.util.Properties;

import com.google.inject.Injector;
import com.google.inject.Module;

import android.app.Application;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;

/**
 * This class extends from Application instead of JoynrApplication, because it is the standard way
 * of creating an application on Android and makes it easier to interact with the GUI.
 */
public class HelloWorldProviderApplication extends Application {

    private JoynrRuntime runtime;

    public JoynrRuntime getRuntime() {
        return runtime;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        final String host = "localhost";
        final int port = 4242;
        final String localDomain = "domain";

        final String STATIC_PERSISTENCE_FILE = "provider-joynr.properties";
        final String STATIC_PARTICIPANTS_FILE = "joynr.properties_participants";
        final String STATIC_SUBSCRIPTION_REQUESTS_FILE = "joynr.subscriptionrequests";
        final String PROPERTY_JOYNR_DOMAIN_LOCAL = "joynr.domain.local";

        final Properties joynrConfig = new Properties();

        final Module runtimeModule = new LibjoynrWebSocketRuntimeModule();

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                                getApplicationContext().getCacheDir() + "/" + STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE,
                                getApplicationContext().getCacheDir() + "/" + STATIC_PARTICIPANTS_FILE);

        joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCE_FILE,
                                getApplicationContext().getCacheDir() + "/" + STATIC_SUBSCRIPTION_REQUESTS_FILE);

        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);

        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, host);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        final Injector joynrInjector = new JoynrInjectorFactory(joynrConfig, runtimeModule).createChildInjector();

        runtime = joynrInjector.getInstance(JoynrRuntime.class);
    }
}
