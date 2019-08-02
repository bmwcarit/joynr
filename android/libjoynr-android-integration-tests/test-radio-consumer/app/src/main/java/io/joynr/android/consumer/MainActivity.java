package io.joynr.android.consumer;

import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        RadioConsumerApp radioConsumerApp = new RadioConsumerApp();
        radioConsumerApp.init(getApplication());

        Button currentStationButton = findViewById(R.id.currentStationButton);
        Button shuffleStationButton = findViewById(R.id.shuffleStationButton);

        currentStationButton.setOnClickListener(
                view -> setCurrentStation(radioConsumerApp.getCurrentStation()));

        shuffleStationButton.setOnClickListener(
                view -> setCurrentStation(radioConsumerApp.shuffleStations()));
    }

    private void setCurrentStation(String text) {

        ((TextView) findViewById(R.id.testText)).setText(text);
        
    }
}
