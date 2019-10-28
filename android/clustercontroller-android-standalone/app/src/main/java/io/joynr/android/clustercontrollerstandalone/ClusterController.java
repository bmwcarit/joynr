/*
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
package io.joynr.android.clustercontrollerstandalone;

import android.content.Context;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLogger;
import org.slf4j.impl.StaticLoggerBinder;

import java.util.Properties;

import io.joynr.runtime.JoynrRuntime;

import static io.joynr.android.AndroidBinderRuntime.initClusterController;

public class ClusterController {

    private static final Logger logger = LoggerFactory.getLogger(ClusterController.class);

    public static JoynrRuntime run(final Context context, final String brokerUri) {

        StaticLoggerBinder.setLogLevel(AndroidLogger.LogLevel.DEBUG);
        logger.debug("Starting...");

        return initClusterController(context, brokerUri, new Properties());
    }

}
