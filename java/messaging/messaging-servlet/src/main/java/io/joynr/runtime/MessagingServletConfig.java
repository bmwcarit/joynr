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

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.Provides;
import com.google.inject.Scopes;
import io.joynr.JoynrApplicationLauncher;
import io.joynr.guice.LowerCaseProperties;
import io.joynr.guice.servlet.AbstractJoynrServletModule;
import io.joynr.messaging.IServletMessageReceivers;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingService;
import io.joynr.messaging.ServletMessageReceivers;
import io.joynr.messaging.ServletMessagingModule;
import io.joynr.messaging.ServletPropertyLoader;
import io.joynr.servlet.JoynrWebServlet;

import java.util.Properties;
import java.util.Set;

import javax.servlet.ServletContext;
import javax.servlet.ServletContextEvent;
import javax.servlet.http.HttpServlet;

import org.apache.commons.lang.ArrayUtils;
import org.reflections.Reflections;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.jaxrs.json.JacksonJsonProvider;
import com.google.inject.servlet.GuiceServletContextListener;
import com.google.inject.util.Modules;
import com.sun.jersey.guice.JerseyServletModule;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

/**
 *
 * http://code.google.com/p/google-guice/wiki/ServletModule
 *
 * To use this class to configue guice binding within jersey, add it as a listener in the web.xml file
 *
 * <pre>
 * {@code
 *
 *  <listener>
 *      <listener-class>io.joynr.runtime.MessagingServletConfig</listener-class>
 *  </listener>
 * }
 * </pre>
 *
 *
 */

public class MessagingServletConfig extends GuiceServletContextListener {
    public static final String INIT_PARAM_SERVLET_MODULE_CLASSNAME = "servletmodule";

    private static final String IO_JOYNR_APPS_PACKAGES = "io.joynr.apps.packages";
    private static final String DEFAULT_SERVLET_MODULE_NAME = "io.joynr.servlet.ServletModule";
    private static final String DEFAULT_SERVLET_MESSAGING_PROPERTIES = "defaultServletMessaging.properties";

    private String servletModuleName;

    private static final Logger logger = LoggerFactory.getLogger(MessagingServletConfig.class);

    private JerseyServletModule jerseyServletModule;
    private IServletMessageReceivers messageReceivers = new ServletMessageReceivers();
    private JoynrApplicationLauncher appLauncher = null;

    @Override
    public void contextInitialized(ServletContextEvent servletContextEvent) {

        ServletContext servletContext = servletContextEvent.getServletContext();

        // properties from appProperties will extend and override the default
        // properties
        Properties properties = new LowerCaseProperties(PropertyLoader.loadProperties(DEFAULT_SERVLET_MESSAGING_PROPERTIES));
        String appPropertiesFileName = servletContext.getInitParameter("properties");
        if (appPropertiesFileName != null) {
            Properties appProperties = ServletPropertyLoader.loadProperties(appPropertiesFileName, servletContext);
            properties.putAll(appProperties);
        } else {
            logger.warn("to load properties, set the initParameter 'properties' ");
        }

        // add OS environment properties
        properties.putAll(System.getenv());

        servletModuleName = properties.getProperty(INIT_PARAM_SERVLET_MODULE_CLASSNAME);
        servletModuleName = servletModuleName == null ? DEFAULT_SERVLET_MODULE_NAME : servletModuleName;

        // create a reflection set used to find
        // * all plugin application classes implementing the JoynApplication interface
        // * all servlets annotated as WebServlet
        // * the class implementing JoynrInjectorFactory (should be only one)
        Object[] appPackages = mergeAppPackages(properties);

        // Add Java system properties (set with -D)
        properties.putAll(System.getProperties());

        // TODO if reflections is ever a performance problem, can statically scan and save the reflections data using
        // Maven,
        // then work on the previously scanned data
        // see: https://code.google.com/p/reflections/wiki/UseCases
        Reflections reflections = new Reflections("io.joynr.runtime", appPackages);
        final Set<Class<?>> classesAnnotatedWithWebServlet = reflections.getTypesAnnotatedWith(JoynrWebServlet.class);
        final Set<Class<?>> classesAnnotatedWithProvider = reflections.getTypesAnnotatedWith(javax.ws.rs.ext.Provider.class);

        // The jerseyServletModule injects the servicing classes using guice,
        // instead of letting jersey do it natively
        jerseyServletModule = new AbstractJoynrServletModule() {

            @SuppressWarnings("unchecked")
            @Override
            protected void configureJoynrServlets() {
                bind(GuiceContainer.class);
                bind(JacksonJsonProvider.class).asEagerSingleton();

                bind(MessagingService.class);
                //get all classes annotated with @Provider and bind them
                for (Class<?> providerClass : classesAnnotatedWithProvider) {
                    bind(providerClass).in(Scopes.SINGLETON);
                }

                for (Class<?> webServletClass : classesAnnotatedWithWebServlet) {
                    if (!HttpServlet.class.isAssignableFrom(webServletClass)) {
                        continue;
                    }

                    bind(webServletClass);
                    JoynrWebServlet webServletAnnotation = webServletClass.getAnnotation(JoynrWebServlet.class);
                    if (webServletAnnotation == null) {
                        logger.error("webServletAnnotation was NULL.");
                        continue;
                    }
                    serve(webServletAnnotation.value()).with((Class<? extends HttpServlet>) webServletClass);
                }
            }

            // this @SuppressWarnings is needed for the build on jenkins
            @SuppressWarnings("unused")
            @Provides
            public IServletMessageReceivers providesMessageReceivers() {
                return messageReceivers;
            }

        };
        Class<?> servletModuleClass = null;
        Module servletModule = null;
        try {
            servletModuleClass = this.getClass().getClassLoader().loadClass(servletModuleName);
            servletModule = (Module) servletModuleClass.newInstance();
        } catch (ClassNotFoundException e) {
            logger.debug("Servlet module class not found will use default configuration");
        } catch (Exception e) {
            logger.error("", e);
        }

        if (servletModule == null) {
            servletModule = new EmptyModule();
        }

        properties.put(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT, servletContext.getContextPath());

        String hostPath = properties.getProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH);
        if (hostPath == null) {
            hostPath = properties.getProperty("hostpath");
        }
        if (hostPath != null) {
            properties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, hostPath);
        }

        // find all plugin application classes implementing the JoynApplication interface
        Set<Class<? extends JoynrApplication>> joynrApplicationsClasses = reflections.getSubTypesOf(JoynrApplication.class);
        Set<Class<? extends AbstractJoynrInjectorFactory>> joynrInjectorFactoryClasses = reflections.getSubTypesOf(AbstractJoynrInjectorFactory.class);
        assert (joynrInjectorFactoryClasses.size() == 1);

        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
            }
        });
        AbstractJoynrInjectorFactory injectorFactory = injector.getInstance(joynrInjectorFactoryClasses.iterator()
                                                                                                       .next());
        appLauncher = injector.getInstance(JoynrApplicationLauncher.class);

        logger.info("starting joynr with properties: {}", properties);

        appLauncher.init(properties,
                         joynrApplicationsClasses,
                         injectorFactory,
                         Modules.override(jerseyServletModule, new CCInProcessRuntimeModule())
                                .with(servletModule, new ServletMessagingModule()));

        super.contextInitialized(servletContextEvent);
    }

    private String[] mergeAppPackages(Properties properties) {
        String systemAppPackagesSetting = System.getProperties().getProperty(IO_JOYNR_APPS_PACKAGES);
        String appPackagesSetting = properties.getProperty(IO_JOYNR_APPS_PACKAGES);
        String[] appPackages = appPackagesSetting != null ? appPackagesSetting.split(";") : null;
        String[] systemAppPackages = systemAppPackagesSetting != null ? systemAppPackagesSetting.split(";") : null;
        appPackages = (String[]) ArrayUtils.addAll(appPackages, systemAppPackages);
        return appPackages;
    }

    @Override
    public void contextDestroyed(ServletContextEvent servletContextEvent) {
        // TODO when should be set clear?
        if (appLauncher != null) {
            appLauncher.shutdown(true);
        } else {
            String msg = "Context cannot be destroyed, as bootstrapUtil has not been initialied correctly";
            logger.error(msg);
            throw new IllegalStateException(MessagingServletConfig.class.getSimpleName() + ":" + msg);
        }
        super.contextDestroyed(servletContextEvent);
    }

    @Override
    protected Injector getInjector() {
        return appLauncher.getJoynrInjector();
    }

}
