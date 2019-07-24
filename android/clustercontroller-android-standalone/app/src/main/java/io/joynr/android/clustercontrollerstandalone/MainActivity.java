package io.joynr.android.clustercontrollerstandalone;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends AppCompatActivity {

    /**
     * When this activity is started and this extra is passed in the intent extras,
     * the CC is automatically started with the value of this extra.
     */
    public static final String EXTRA_BROKER_URI = "EXTRA_BROKER_URI";
    private Button button;
    private EditText edittext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        button = findViewById(R.id.startbutton);
        button.setOnClickListener(buttonClickListener);

        edittext = findViewById(R.id.edittext);

        String brokerUri = getIntent().getStringExtra(EXTRA_BROKER_URI);
        if (brokerUri != null) {
            edittext.setText(brokerUri);
            button.performClick();
        }

    }

    private View.OnClickListener buttonClickListener = v -> {

        if (button.getText().toString().equalsIgnoreCase(getString(R.string.start))) {
            startService();
            button.setText(R.string.stop);
            edittext.setEnabled(false);
        } else {
            stopService();
            button.setText(R.string.start);
            edittext.setEnabled(true);
        }
    };


    public void startService() {

        String editTextBrokerUri = edittext.getText().toString();

        Intent serviceIntent = new Intent(this, ClusterControllerService.class);
        serviceIntent.putExtra(EXTRA_BROKER_URI, editTextBrokerUri);

        startService(serviceIntent);
    }

    public void stopService() {
        Intent serviceIntent = new Intent(this, ClusterControllerService.class);
        stopService(serviceIntent);
    }

}
