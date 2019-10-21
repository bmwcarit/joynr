package io.joynr.android.consumer;

import android.app.Application;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLogger;
import org.slf4j.impl.StaticLoggerBinder;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.runtime.JoynrRuntime;
import joynr.MulticastSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.vehicle.RadioBroadcastInterface;
import joynr.vehicle.RadioProxy;
import joynr.vehicle.RadioStation;

public class RadioConsumerApp extends Application {

    private static final Logger logger = LoggerFactory.getLogger(RadioConsumerApp.class);

    private static final String RADIO_LOCAL_DOMAIN = "radio.local.domain";

    private JoynrRuntime runtime;
    private RadioProxy radioProxy;
    private Future<String> futureWeakSignal;


    @Override
    public void onCreate() {
        super.onCreate();

        //set loglevel to debug
        StaticLoggerBinder.setLogLevel(AndroidLogger.LogLevel.DEBUG);

        runtime = AndroidBinderRuntime.init(this);

        registerProxy();
    }


    public void registerProxy() {

        logger.debug("Starting...");

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        radioProxy = runtime.getProxyBuilder(RADIO_LOCAL_DOMAIN, RadioProxy.class)
                .setDiscoveryQos(discoveryQos)
                .build();

    }

    String getCurrentStation() {
        return radioProxy.getCurrentStation().getName();

    }

    String shuffleStations() {
        radioProxy.shuffleStations();
        return getCurrentStation();
    }

    void unsubscribeFromSubscription(){

        try {
            radioProxy.unsubscribeFromWeakSignalBroadcast(futureWeakSignal.get());
        } catch (InterruptedException e) {
            e.printStackTrace();
        } catch (ApplicationException e) {
            e.printStackTrace();
        }
    }

    void subscribeToWeakSignal(){
        MulticastSubscriptionQos weakSignalBroadcastSubscriptionQos;
        weakSignalBroadcastSubscriptionQos = new MulticastSubscriptionQos();
        futureWeakSignal = radioProxy.subscribeToWeakSignalBroadcast(new RadioBroadcastInterface.WeakSignalBroadcastAdapter() {
            @Override
            public void onReceive(RadioStation weakSignalStation) {
                logger.info( "BROADCAST SUBSCRIPTION: weak signal: " + weakSignalStation);
            }
        }, weakSignalBroadcastSubscriptionQos);

    }

    private void subscribeAttributeStation(){

        int minInterval_ms = 0;
        int maxInterval_ms = 10000;

        long validityMs = 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
        subscriptionQos.setMinIntervalMs(minInterval_ms).setMaxIntervalMs(maxInterval_ms).setValidityMs(validityMs);
        subscriptionQos.setAlertAfterIntervalMs(alertAfterInterval_ms).setPublicationTtlMs(publicationTtl_ms);

        Future<String> subscriptionFutureCurrentStation;

        // subscribe to an attribute
        radioProxy.subscribeToCurrentStation(new AttributeSubscriptionAdapter<RadioStation>() {

            @Override
            public void onReceive(RadioStation value) {
                logger.info("SUBSCRIPTION: current station: " + value);
            }

            @Override
            public void onError(JoynrRuntimeException error) {
                logger.info("ERROR SUBSCRIPTION: " + error);
            }
        }, subscriptionQos);
    }

}
