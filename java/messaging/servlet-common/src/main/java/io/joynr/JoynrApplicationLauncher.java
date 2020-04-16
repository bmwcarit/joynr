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
package io.joynr;

import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;
import com.google.inject.Module;

import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.runtime.AbstractJoynrInjectorFactory;
import io.joynr.runtime.AcceptsMessageReceiver;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.MessageReceiverType;

/**
 * Used to start joynr applications using a common injector
 *
 */
public class JoynrApplicationLauncher {

    private static final Logger logger = LoggerFactory.getLogger(JoynrApplicationLauncher.class);
    private ExecutorService executionQueue;
    private List<JoynrApplication> apps = new ArrayList<JoynrApplication>();
    private static final long timeout = 20;
    private Injector joynrInjector;

    public void init(Properties properties,
                     Set<Class<? extends JoynrApplication>> joynrApplicationsClasses,
                     AbstractJoynrInjectorFactory injectorFactory,
                     Module... modules) {
        ThreadFactory threadFactory = new ThreadFactory() {
            @Override
            public Thread newThread(Runnable r) {
                return new Thread(r, r.getClass().getSimpleName());
            }
        };

        executionQueue = Executors.newCachedThreadPool(threadFactory);

        try {

            // updateInjectorModule must be called before getInjector can be called.
            // for this reason the injector has to be created here, and not by the creator of the injectorFactory
            injectorFactory.updateInjectorModule(properties, modules);

            joynrInjector = injectorFactory.getInjector();

            for (Class<? extends JoynrApplication> appClass : joynrApplicationsClasses) {
                if (Modifier.isAbstract(appClass.getModifiers())) {
                    continue;
                }

                AcceptsMessageReceiver acceptsMessageReceiverAnnotation = appClass.getAnnotation(AcceptsMessageReceiver.class);
                // TODO why not reuse the previously requested value for
                // AcceptsMessageReceiver?
                MessageReceiverType messageReceiverType = acceptsMessageReceiverAnnotation == null
                        ? MessageReceiverType.ANY
                        : appClass.getAnnotation(AcceptsMessageReceiver.class).value();
                if (messageReceiverType.equals(MessageReceiverType.SERVLET)) {
                    continue;
                }

                logger.info("Running app: {}", appClass.getName());
                final JoynrApplication app = injectorFactory.createApplication(new JoynrApplicationModule(appClass));
                apps.add(app);
                executionQueue.submit(app);
            }
        } catch (RuntimeException e) {
            logger.error("ERROR", e);
            throw e;
        }

    }

    /**
     * If clear, then deregister etc.
     *
     * @param clear
     *   indicates whether the messageListener of the servlet receiver
     *   should be dropped and the channel closed
     */
    // TODO support clear properly
    public void shutdown(boolean clear) {
        if (executionQueue != null) {
            executionQueue.shutdownNow();
        }

        if (joynrInjector != null) {
            // switch to lp receiver and call servlet shutdown to be able to receive responses
            ServletMessageReceiver servletReceiver = joynrInjector.getInstance(ServletMessageReceiver.class);
            servletReceiver.switchToLongPolling();

            for (JoynrApplication app : apps) {
                try {
                    app.shutdown();
                } catch (Exception e) {
                    logger.debug("Error shutting down app {}: {}", app.getClass(), e);
                }
            }
            servletReceiver.shutdown(clear);
        }
        try {
            if (executionQueue != null) {
                executionQueue.awaitTermination(timeout, TimeUnit.SECONDS);
            }
        } catch (InterruptedException e) {
            return;
        }

    }

    public Injector getJoynrInjector() {
        return joynrInjector;
    }

}
