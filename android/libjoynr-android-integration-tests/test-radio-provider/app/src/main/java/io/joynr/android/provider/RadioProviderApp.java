package io.joynr.android.provider;

import static io.joynr.runtime.AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL;

import android.content.Context;
import android.util.Log;

import java.util.Properties;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.Future;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class RadioProviderApp {

    private static final String TAG = RadioProviderApp.class.getSimpleName();


    public static final String STATIC_PERSISTENCE_FILE = "provider-joynr.properties";
    public static final String STATIC_PARTICIPANTS_FILE = "joynr.properties_participants";
    public static final String STATIC_SUBSCRIPTION_REQUESTS_FILE = "joynr.subscriptionrequests";

    private static final String RADIO_LOCAL_DOMAIN = "radio.local.domain";
    private static final String CC_HOST = "localhost";
    private static final int CC_PORT = 4242;


    public void init(Context context) {

        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, context.getCacheDir() + "/" + STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                context.getCacheDir() + "/" + STATIC_PARTICIPANTS_FILE);

        joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_SUBSCRIPTIONREQUESTS_PERSISISTENCE_FILE,
                context.getCacheDir() + "/" + STATIC_SUBSCRIPTION_REQUESTS_FILE);


        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, RADIO_LOCAL_DOMAIN);

        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, CC_HOST);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + CC_PORT);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        JoynrRuntime runtime = new JoynrInjectorFactory(joynrConfig, new LibjoynrWebSocketRuntimeModule())
                .createChildInjector().getInstance(JoynrRuntime.class);

        RadioProvider provider = new RadioProvider();
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(System.currentTimeMillis());
        providerQos.setScope(ProviderScope.LOCAL);
        boolean awaitGlobalRegistration = true;
        Future<Void> future = runtime.registerProvider(RADIO_LOCAL_DOMAIN, provider, providerQos, awaitGlobalRegistration);
        try {
            future.get();
        } catch (Exception e) {
            Log.e(TAG, "runtime.registerProvider failed: ", e);
        }

    }

}
