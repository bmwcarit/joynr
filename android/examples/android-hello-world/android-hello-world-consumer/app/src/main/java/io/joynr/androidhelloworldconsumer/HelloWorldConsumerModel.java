package io.joynr.androidhelloworldconsumer;

import android.util.Log;

import androidx.lifecycle.MutableLiveData;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.helloworld.HelloWorldProxy;

public class HelloWorldConsumerModel {

    private static final String PRINT_BORDER = "\n####################\n";
    public MutableLiveData<String> text = new MutableLiveData<>();
    private Future<String> futureString;
    private HelloWorldConsumerApplication app;
    private HelloWorldProxy proxy;

    public void setApp(final HelloWorldConsumerApplication app) {
        this.app = app;
    }

    private void updateString() {
        try {
            text.postValue(futureString.get(300));
        } catch (final InterruptedException e) {
            e.printStackTrace();
        } catch (final ApplicationException e) {
            e.printStackTrace();
        }
    }

    public void onClick() {
        futureString = this.proxy.getHello(
                new Callback<String>() {
                    @Override
                    public void onSuccess(final String s1) {
                        Log.d("Received Message", PRINT_BORDER + " Received: " + s1 + PRINT_BORDER);
                    }

                    @Override
                    public void onFailure(final JoynrRuntimeException e) {
                        Log.d("Failed Message","Failed to receive message with the error: " + e);
                    }
                }
        );
        updateString();
    }

    public void init() {
        final DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(10000);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        final int minInterval_ms = 0;
        final int maxInterval_ms = 10000;
        final long validityMs = 60000;
        final int alertAfterInterval_ms = 20000;
        final int publicationTtl_ms = 5000;

        final OnChangeWithKeepAliveSubscriptionQos subscriptionQos =
                new OnChangeWithKeepAliveSubscriptionQos();
        subscriptionQos.setMinIntervalMs(minInterval_ms).setMaxIntervalMs(maxInterval_ms).setValidityMs(validityMs);
        subscriptionQos.setAlertAfterIntervalMs(alertAfterInterval_ms).setPublicationTtlMs(publicationTtl_ms);

        proxy = app.getRuntime().getProxyBuilder("domain", HelloWorldProxy.class)
                .setMessagingQos(new MessagingQos()).setDiscoveryQos(discoveryQos).build();

    }
}
