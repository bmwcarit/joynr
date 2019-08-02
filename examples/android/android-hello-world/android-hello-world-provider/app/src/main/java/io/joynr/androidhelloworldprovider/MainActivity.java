package io.joynr.androidhelloworldprovider;

import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProviders;

import java.util.List;

public class MainActivity extends AppCompatActivity {

    private HelloWorldProviderViewModel myProviderViewModel;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        myProviderViewModel = ViewModelProviders.of(this).get(HelloWorldProviderViewModel.class);

        myProviderViewModel.init((HelloWorldProviderApplication) getApplication());

        TextView view = findViewById(R.id.text);
        view.setMovementMethod(new ScrollingMovementMethod());

        bindData();

    }

    public void bindData() {
        myProviderViewModel.onUpdatedString().observe(this,this::updateText);
    }

    private void updateText(List<String> ls1) {
        TextView view = findViewById(R.id.text);

        String text = "";
        StringBuilder sb = new StringBuilder();
        sb.append(view.getText());

        for (String s: ls1) {
            sb.append(s + "\n");
        }

        view.setText(sb.toString());
    }
}
