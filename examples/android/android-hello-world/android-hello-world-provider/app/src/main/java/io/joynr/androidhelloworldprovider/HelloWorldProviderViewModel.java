package io.joynr.androidhelloworldprovider;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.ViewModel;

import java.util.List;

public class HelloWorldProviderViewModel extends ViewModel {
    private final HelloWorldProviderModel mpm = new HelloWorldProviderModel();

    public void init(final HelloWorldProviderApplication app) {
        mpm.setApp(app);
        mpm.init();
    }

    public LiveData<List<String>> onUpdatedString() {
        return mpm.getData();
    }
}
