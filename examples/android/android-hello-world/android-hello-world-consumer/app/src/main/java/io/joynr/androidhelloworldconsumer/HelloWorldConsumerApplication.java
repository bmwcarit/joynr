package io.joynr.androidhelloworldconsumer;

import android.app.Application;

import com.google.inject.Injector;
import com.google.inject.Module;

import java.util.Properties;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;

/**
 * This class extends from Application instead of JoynrApplication, because it is the standard way
 * of creating an application on Android and makes it easier to interact with the GUI.
 */

public class HelloWorldConsumerApplication extends Application {

    public JoynrRuntime runtime;

    public JoynrRuntime getRuntime() {
        return runtime;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        final String host = "localhost";
        final int port = 4242;

        final String STATIC_PERSISTENCE_FILE = "consumer-joynr.properties";

        final Properties joynrConfig = new Properties();
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, host);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "/");

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                getApplicationContext().getCacheDir() + "/" + STATIC_PERSISTENCE_FILE);

        final Module runtimeModule = new LibjoynrWebSocketRuntimeModule();

        final Injector joynrInjector =
                new JoynrInjectorFactory(joynrConfig, runtimeModule).createChildInjector();

        runtime = joynrInjector.getInstance(JoynrRuntime.class);

    }
}
