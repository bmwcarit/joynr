package io.joynr.androidhelloworldconsumer;

import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProviders;

public class MainActivity extends AppCompatActivity {

    private HelloWorldConsumerViewModel mcvm;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mcvm = ViewModelProviders.of(this).get(HelloWorldConsumerViewModel.class);
        mcvm.init((HelloWorldConsumerApplication) getApplication());

        final Button button = findViewById(R.id.button);

        button.setOnClickListener(view -> mcvm.onClick());
        bindData();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    public void bindData() {
        mcvm.onUpdatedString().observe(this, this::updateText);
    }

    private void updateText(final String text) {
        final TextView view = findViewById(R.id.text_box);
        view.setText(text);
    }

}
