package io.joynr.guice.servlet;

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

import java.util.Arrays;
import java.util.Properties;

import javax.servlet.Filter;
import javax.servlet.annotation.WebFilter;

import io.joynr.runtime.PropertyLoader;
import io.joynr.servlet.DefaultServletWrapper;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Singleton;
import com.google.inject.servlet.GuiceFilter;
import com.sun.jersey.core.spi.scanning.PackageNamesScanner;
import com.sun.jersey.guice.JerseyServletModule;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;
import com.sun.jersey.spi.scanning.AnnotationScannerListener;

/**
 * Common base class for all joynr servlet modules.
 * 
 */
public abstract class AbstractJoynrServletModule extends JerseyServletModule {

    private static final Logger logger = LoggerFactory.getLogger(AbstractJoynrServletModule.class);

    private static final String IO_JOYNR_APPS_PACKAGES = "io.joynr.apps.packages";

    @Override
    final protected void configureServlets() {

        // GuiceFilter is instantiated here instead of configured in
        // web.xml to allow for more than one servlet per servlet
        // container.
        // See documentation of web.xml for details.
        bind(GuiceFilter.class);

        configureJoynrServlets();

        // Route all requests through GuiceContainer
        bindStaticWebResources();
        bindJoynrServletClass();

        bindAnnotatedFilters();
    }

    /**
     * Binds all static web resources such as html pages or images to {@link DefaultServletWrapper}.
     */
    protected void bindStaticWebResources() {
        // Route html, js, jpg, png, css requests through GuiceContainer
        bind(DefaultServletWrapper.class);
        serve("*.html", "*.htm", "*.js", "*.jpg", "*.png", "*.css").with(DefaultServletWrapper.class);
    }

    /**
     * Binds the servlet class by calling something like
     * {@code serve("/*").with(servletClass>)}. The default implementation binds
     * {@link GuiceContainer} to serve all URLs.
     */
    protected void bindJoynrServletClass() {
        serve("/*").with(GuiceContainer.class);
    }

    /**
     * Configures the joynr servlets. <br>
     * It is expected to register at least one Jersey resource by calling
     * {@code bind} for Jersey resource classes. Filters can be applied by
     * calling {@code filter}.
     */
    protected abstract void configureJoynrServlets();

    /**
     * Sets up filters that are annotated with the {@link WebFilter} annotation.
     * Every class in the classpath is searched for the annotation.
     */
    @SuppressWarnings("unchecked")
    private void bindAnnotatedFilters() {

        String appsPackages = null;
        if (System.getProperties().containsKey(IO_JOYNR_APPS_PACKAGES)) {
            logger.info("Using property {} from system properties", IO_JOYNR_APPS_PACKAGES);
            appsPackages = System.getProperty(IO_JOYNR_APPS_PACKAGES);
        } else {
            Properties servletProperties = PropertyLoader.loadProperties("servlet.properties");
            if (servletProperties.containsKey(IO_JOYNR_APPS_PACKAGES)) {
                appsPackages = servletProperties.getProperty(IO_JOYNR_APPS_PACKAGES);
            }
        }

        if (appsPackages != null) {
            String[] packageNames = appsPackages.split(";");
            logger.info("Searching packages for @WebFilter annotation: {}", Arrays.toString(packageNames));

            PackageNamesScanner scanner = new PackageNamesScanner(packageNames);
            AnnotationScannerListener sl = new AnnotationScannerListener(WebFilter.class);
            scanner.scan(sl);

            for (Class<?> webFilterAnnotatedClass : sl.getAnnotatedClasses()) {

                if (Filter.class.isAssignableFrom(webFilterAnnotatedClass)) {
                    bind(webFilterAnnotatedClass).in(Singleton.class);
                    filter("/*").through((Class<? extends Filter>) webFilterAnnotatedClass);
                    logger.info("Adding filter {} for '/*'", webFilterAnnotatedClass.getName());
                }

            }
        }
    }

}
