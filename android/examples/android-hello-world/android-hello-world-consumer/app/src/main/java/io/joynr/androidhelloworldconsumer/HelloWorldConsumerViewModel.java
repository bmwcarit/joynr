package io.joynr.androidhelloworldconsumer;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.ViewModel;

public class HelloWorldConsumerViewModel extends ViewModel {
    private final HelloWorldConsumerModel mca = new HelloWorldConsumerModel();

    public void onClick() {
        mca.onClick();
    }

    public void init(final HelloWorldConsumerApplication app) {
        mca.setApp(app);
        mca.init();
    }

    public LiveData<String> onUpdatedString() {
        return mca.text;
    }
}
