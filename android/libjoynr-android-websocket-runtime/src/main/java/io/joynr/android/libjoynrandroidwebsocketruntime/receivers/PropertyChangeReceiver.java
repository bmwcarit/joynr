/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.android.libjoynrandroidwebsocketruntime.receivers;

import java.util.Objects;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLogger;
import org.slf4j.impl.StaticLoggerBinder;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class PropertyChangeReceiver extends BroadcastReceiver {
    public static final String ACTION_LOG_LEVEL_CHANGE = "io.joynr.android.LOG_LEVEL_CHANGE";
    public static final String LOG_LEVEL_TAG = "setlevel";
    private static final Logger logger = LoggerFactory.getLogger(PropertyChangeReceiver.class);

    @Override
    public void onReceive(final Context context, final Intent intent) {
        if (Objects.equals(intent.getAction(), ACTION_LOG_LEVEL_CHANGE)) {
            if (intent.hasExtra(LOG_LEVEL_TAG)) {
                final String logLevelExtra = intent.getStringExtra(LOG_LEVEL_TAG);
                final @AndroidLogger.LogLevel int logLevel = StaticLoggerBinder.getValidLogLevel(logLevelExtra);
                StaticLoggerBinder.setLogLevel(logLevel);

                logger.info(String.format("Set log level to %1$s", logLevelExtra));
            }
        }
    }
}
