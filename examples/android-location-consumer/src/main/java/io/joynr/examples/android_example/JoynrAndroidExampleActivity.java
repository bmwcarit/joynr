package io.joynr.examples.android_example;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

import io.joynr.messaging.websocket.WebsocketModule;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

public class JoynrAndroidExampleActivity extends Activity {
    private static final Logger logger = LoggerFactory.getLogger(JoynrAndroidExampleActivity.class);
    private static final String TAG = "joynrAndroidExample";
    private JoynrAndroidExampleApplication application;

    /**
     * Called when the activity is first created.
     * 
     * @param savedInstanceState
     *            If the activity is being re-initialized after previously being shut down then this Bundle contains the
     *            data it most recently supplied in onSaveInstanceState(Bundle). <b>Note: Otherwise it is null.</b>
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        logger.info("onCreate JoynrAndroidExampleActivity");
        Log.i(TAG, "onCreate");
        setContentView(R.layout.main);
        application = (JoynrAndroidExampleApplication) getApplication();

        application.setOutput(new Output() {
            TextView textView = (TextView) findViewById(R.id.progressoutput);

            @Override
            public void append(final String text) {
                // Android prohibits access UI elements from outside the UI thread
                // with "someUiElement".post(..) you can pass a runnable to the ui thread to modify UI content.
                // see http://developer.android.com/guide/components/processes-and-threads.html
                textView.post(new Runnable() {

                    @Override
                    public void run() {
                        textView.append(text);

                    }
                });
            }

        });

        Properties joynrConfig = new Properties();
        String clusterControllerUrlString = "ws://10.0.2.2:4242";
        try {
            URI clusterControllerUrl = new URI(clusterControllerUrlString);
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, clusterControllerUrl.getHost());
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, ""
                    + clusterControllerUrl.getPort());
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL,
                                    clusterControllerUrl.getScheme());
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
        } catch (URISyntaxException e) {
            logger.info("Cluster Controller WebSocket URL is invalid: " + clusterControllerUrlString + " error: "
                    + e.getMessage());
        }
        application.initJoynrRuntime(joynrConfig);
    }

    public void onCreateProxyClicked(View view) {
        // Requesting the GPS Location via joynr will take some seconds -> it has to be executed in a new thread (not the
        // UI thread) otherwise the device would not react on user interaction until the request is completed or the
        // application is even stopped with an "Application Not Responding" dialog
        // see http://developer.android.com/guide/components/processes-and-threads.html
        new Thread(new Runnable() {
            public void run() {
                application.createProxy();
            }
        }).start();
    }

    public void onGetGpsLocationClicked(View view) {
        application.getGpsLocation();
    }

    public void onCreateGpsLocationSubscriptionClicked(View view) {
        application.subscribeToGpsLocation();
    }
}
