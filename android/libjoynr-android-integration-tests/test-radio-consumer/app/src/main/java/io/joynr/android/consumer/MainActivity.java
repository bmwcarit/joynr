package io.joynr.android.consumer;

import android.os.Bundle;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        findViewById(R.id.subscribeToWeakSignalButton).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(){
                    @Override
                    public void run() {
                        super.run();
                        ((RadioConsumerApp)getApplication()).subscribeToWeakSignal();
                    }
                }.start();
            }
        });

        findViewById(R.id.unsubscribeWeakSignalButton).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(){
                    @Override
                    public void run() {
                        super.run();
                        ((RadioConsumerApp)getApplication()).unsubscribeFromSubscription();
                    }
                }.start();
            }
        });



    }

}
