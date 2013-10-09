package io.joynr;

/*
 * #%L
 * joynr::java::messaging::servletcommon
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.runtime.AbstractJoynrInjectorFactory;
import io.joynr.runtime.AcceptsMessageReceiver;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.MessageReceiverType;

import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.reflections.Reflections;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;
import com.google.inject.Module;

/**
 * Scans all subpackages under "de.bmw" for instances of AbstractJoynApplication, which are then injected and run.
 * 
 * @author david.katz
 * 
 */
public class BootstrapUtil {

    private static final Logger logger = LoggerFactory.getLogger(BootstrapUtil.class);
    private static ExecutorService executionQueue;
    private static List<JoynrApplication> apps = new ArrayList<JoynrApplication>();
    private static final long timeout = 20;
    private static Injector joynrInjector;

    public static Injector init(Injector injector, Properties properties, Module... modules) {
        ThreadFactory threadFactory = new ThreadFactory() {
            @Override
            public Thread newThread(Runnable r) {
                return new Thread(r, r.getClass().getSimpleName());
            }
        };

        executionQueue = Executors.newCachedThreadPool(threadFactory);

        // find all plugin application classes implementing the JoynApplication interface
        String appPackagesSetting = System.getProperty("io.joynr.apps.packages");

        String[] appPackages = null;
        if (appPackagesSetting != null) {
            appPackages = appPackagesSetting.split(";");
        }
        Reflections reflections = new Reflections("io.joynr", appPackages);
        Set<Class<? extends JoynrApplication>> joynrApplicationsClasses = reflections.getSubTypesOf(JoynrApplication.class);

        Set<Class<? extends AbstractJoynrInjectorFactory>> joynrInjectorFactoryClasses = reflections.getSubTypesOf(AbstractJoynrInjectorFactory.class);
        assert (joynrInjectorFactoryClasses.size() == 1);

        AbstractJoynrInjectorFactory injectorFactory = injector.getInstance(joynrInjectorFactoryClasses.iterator()
                                                                                                       .next());

        injectorFactory.updateInjectorModule(properties, modules);

        joynrInjector = injectorFactory.getInjector();

        for (Class<? extends JoynrApplication> appClass : joynrApplicationsClasses) {
            if (Modifier.isAbstract(appClass.getModifiers())) {
                continue;
            }

            AcceptsMessageReceiver acceptsMessageReceiverAnnotation = appClass.getAnnotation(AcceptsMessageReceiver.class);
            // TODO why not reuse the previously requested value for AcceptsMessageReceiver?
            MessageReceiverType messageReceiverType = acceptsMessageReceiverAnnotation == null ? MessageReceiverType.ANY
                    : appClass.getAnnotation(AcceptsMessageReceiver.class).value();
            if (messageReceiverType.equals(MessageReceiverType.SERVLET)) {
                continue;
            }

            logger.info("Running app: " + appClass.getName());
            final JoynrApplication app = injectorFactory.createApplication(new JoynrApplicationModule(appClass));
            apps.add(app);
            executionQueue.submit(app);
        }

        return joynrInjector;
    }

    /**
     * If clear, then deregister etc. 
     * @param clear
     */
    //TODO support clear properly
    public static void shutdown(boolean clear) {
        if (executionQueue != null) {
            executionQueue.shutdownNow();
        }
        if (joynrInjector != null) {
            // switch to lp receiver and call servlet shutdown to be able to receive responses
            ServletMessageReceiver servletReceiver = joynrInjector.getInstance(ServletMessageReceiver.class);
            servletReceiver.switchToLongPolling();
            for (JoynrApplication app : apps) {
                app.shutdown();
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

}
