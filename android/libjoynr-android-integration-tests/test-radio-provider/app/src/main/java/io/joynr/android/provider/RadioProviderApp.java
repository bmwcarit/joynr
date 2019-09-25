package io.joynr.android.provider;

import android.app.Application;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLogger;
import org.slf4j.impl.StaticLoggerBinder;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.proxy.Future;
import io.joynr.runtime.JoynrRuntime;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class RadioProviderApp extends Application {

    private static final String RADIO_LOCAL_DOMAIN = "radio.local.domain";

    private static final Logger logger = LoggerFactory.getLogger(RadioProviderApp.class);

    private RadioProvider provider;
    private JoynrRuntime runtime;

    @Override
    public void onCreate() {
        super.onCreate();

        //set loglevel to debug
        StaticLoggerBinder.setLogLevel(AndroidLogger.LogLevel.DEBUG);

        //init runtime
        runtime = AndroidBinderRuntime.init(this);

        registerProvider();
    }

    public void registerProvider() {

        logger.debug("Starting...");

        provider = new RadioProvider();
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(System.currentTimeMillis());
        providerQos.setScope(ProviderScope.LOCAL);

        Future<Void> future = runtime.getProviderRegistrar(RADIO_LOCAL_DOMAIN, provider)
                .withProviderQos(providerQos)
                .register();

    }


    public RadioProvider getProvider() {
        return provider;
    }
}
