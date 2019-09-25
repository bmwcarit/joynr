package io.joynr.android.provider;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import androidx.appcompat.app.AppCompatActivity;

import org.slf4j.impl.AndroidLogger;
import org.slf4j.impl.StaticLoggerBinder;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        StaticLoggerBinder.setLogLevel(AndroidLogger.LogLevel.DEBUG);

        Button weakSignalButton = findViewById(R.id.weakSignalButton);
        Button newStationButton = findViewById(R.id.newStationButton);


        weakSignalButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                new Thread() {
                    @Override
                    public void run() {
                        super.run();
                        ((RadioProviderApp)getApplication()).getProvider().fireWeakSignalEvent();

                    }
                }.start();

            }
        });

        newStationButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                new Thread() {
                    @Override
                    public void run() {
                        super.run();
                        ((RadioProviderApp)getApplication()).getProvider().fireNewStationDiscoveredEvent();
                    }
                }.start();

            }
        });
    }
}
