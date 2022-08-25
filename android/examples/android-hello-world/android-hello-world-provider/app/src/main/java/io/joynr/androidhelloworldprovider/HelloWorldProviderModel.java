package io.joynr.androidhelloworldprovider;

import androidx.lifecycle.LiveData;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import joynr.exceptions.ApplicationException;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class HelloWorldProviderModel {

    private static final Logger logger = LoggerFactory.getLogger(HelloWorldProviderModel.class);
    private HelloWorldProvider provider;

    private HelloWorldProviderApplication app;

    public void setApp(final HelloWorldProviderApplication app) {
        this.app = app;
    }

    public void init() {
        provider = new HelloWorldProvider();
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        final Future<Void> future = app.getRuntime().registerProvider("domain", provider,
                providerQos);
        try {
            future.get();
        } catch (final JoynrRuntimeException | ApplicationException | InterruptedException e) {
            logger.error("runtime.registerProvider failed: ", e);
        }
    }

    public LiveData<List<String>> getData() {
        LiveData<List<String>> strings = provider.getData();
        provider.clearData();
        return strings;
    }
}
