package io.joynr.runtime;

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

import java.util.LinkedList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Singleton;

@Singleton
public class ShutdownNotifier {
    private static final Logger logger = LoggerFactory.getLogger(ShutdownNotifier.class);
    List<ShutdownListener> shutdownListeners = new LinkedList<>();

    /**
     * register to have the listener's shutdown method called at system shutdown
     * NOTE: no shutdown order is guaranteed.
     * @param shutdownListener
     */
    public void registerForShutdown(ShutdownListener shutdownListener) {
        shutdownListeners.add(shutdownListener);
    }

    public void shutdown() {
        for (ShutdownListener shutdownListener : shutdownListeners) {
            try {
                shutdownListener.shutdown();
            } catch (Exception e) {
                logger.error("error shutting down {}: {}", shutdownListener, e.getMessage());
            }
        }
    }
}