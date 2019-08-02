package io.joynr.androidhelloworldprovider;

import androidx.lifecycle.MutableLiveData;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.helloworld.HelloWorldAbstractProvider;

public class HelloWorldProvider extends HelloWorldAbstractProvider {

    private String hello;
    private ScheduledExecutorService executorService;
    private static final long DELAY_MS = 2000;
    private static int seed = 0;
    private static String strings[] = {"Hello","World","Friend"};
    public MutableLiveData<List<String>> m_requests;
    private List<String> m_requestsArrays;

    HelloWorldProvider() {
        m_requests = new MutableLiveData<>();
        m_requestsArrays = new ArrayList<>();
    }

    public void clearData() {
        m_requestsArrays.clear();
        m_requests.postValue(m_requestsArrays);
    }

    public MutableLiveData<List<String>> getData() {
        return m_requests;
    }

    @Override
    public Promise<Deferred<String>> getHello() {
        Deferred<String> deferred = new Deferred<>();
        if(seed>=3)
            seed = 0;
        hello = strings[seed++];
        deferred.resolve(hello);
        m_requestsArrays.add("Request: getHello, Value: " + hello);
        m_requests.postValue(m_requestsArrays);
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setHello(final String hello) {
        final DeferredVoid deferredVoid = new DeferredVoid();
        this.hello = hello;
        deferredVoid.resolve();
        return new Promise<>(deferredVoid);
    }
}
