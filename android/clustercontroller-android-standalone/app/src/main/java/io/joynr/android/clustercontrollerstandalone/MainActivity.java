package io.joynr.android.clustercontrollerstandalone;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    /**
     * When this activity is started and this extra is passed in the intent extras,
     * the CC is automatically started with the value of this extra.
     */
    public static final String EXTRA_BROKER_URI = "EXTRA_BROKER_URI";
    private static final Logger logger = LoggerFactory.getLogger(MainActivity.class);
    private Button button;
    private EditText edittext;
    private final View.OnClickListener buttonClickListener = v -> {

        if (button.getText().toString().equalsIgnoreCase(getString(R.string.start))) {
            startService();
            button.setText(R.string.stop);
            edittext.setEnabled(false);
            logger.debug("Started Android CC");
        } else {
            stopService();
            button.setText(R.string.start);
            edittext.setEnabled(true);
            logger.debug("Stopped Android CC");
        }
    };

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        button = findViewById(R.id.startbutton);
        button.setOnClickListener(buttonClickListener);

        edittext = findViewById(R.id.edittext);

        final String brokerUri = getIntent().getStringExtra(EXTRA_BROKER_URI);
        if (brokerUri != null) {
            edittext.setText(brokerUri);
            button.performClick();
        }
    }

    public void startService() {

        final String editTextBrokerUri = edittext.getText().toString();

        final Intent serviceIntent = new Intent(this, ClusterControllerService.class);
        serviceIntent.putExtra(EXTRA_BROKER_URI, editTextBrokerUri);

        startService(serviceIntent);
    }

    public void stopService() {
        final Intent serviceIntent = new Intent(this, ClusterControllerService.class);
        stopService(serviceIntent);
    }

}
