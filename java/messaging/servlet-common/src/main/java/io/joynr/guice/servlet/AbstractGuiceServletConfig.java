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

import java.util.List;

import javax.servlet.ServletContextEvent;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.servlet.GuiceServletContextListener;

/**
 * This class contains methods equals to all servlets that are configured with
 * Guice.
 */
public abstract class AbstractGuiceServletConfig extends GuiceServletContextListener {

    private Injector injector;

    /*
     * (non-Javadoc)
     * 
     * @see
     * com.google.inject.servlet.GuiceServletContextListener#contextInitialized
     * (javax.servlet.ServletContextEvent)
     */
    @Override
    public void contextInitialized(ServletContextEvent servletContextEvent) {

        // The jerseyServletModule injects the servicing classes using guice,
        // instead of letting jersey do it natively
        AbstractJoynrServletModule jerseyServletModule = getJoynrServletModule();
        List<Module> joynrModules = getJoynrModules();
        joynrModules.add(jerseyServletModule);

        injector = Guice.createInjector(joynrModules);

        //registerGuiceFilter(servletContextEvent);

        super.contextInitialized(servletContextEvent);
    }

    /**
     * Registers the Guice filter that is usually defined in web.xml
     * programmatically.
     * 
     */
    //    private void registerGuiceFilter(ServletContextEvent servletContextEvent) {
    //
    //        final String filterName = "guiceFilter";
    //        GuiceFilter filter = injector.getInstance(GuiceFilter.class);
    //
    //        ServletContext context = servletContextEvent.getServletContext();
    //        context.addFilter(filterName, filter);
    //        FilterRegistration filterRegistration = context.getFilterRegistrations().get(filterName);
    //        filterRegistration.setInitParameter("async-supported", "true");
    //        filterRegistration.addMappingForUrlPatterns(EnumSet.allOf(DispatcherType.class), true, "/*");
    //    }

    /*
     * (non-Javadoc)
     * 
     * @see com.google.inject.servlet.GuiceServletContextListener#getInjector()
     */
    @Override
    protected Injector getInjector() {
        return injector;
    }

    /**
     * Returns a servlet module that configures servlet resources with Guice.
     * I.e. it defines which Jersey resources are bound, which filters are
     * applied and which URLs are served.
     * 
     * @return a servlet module that defines bindings for servlet resources.
     */
    protected abstract AbstractJoynrServletModule getJoynrServletModule();

    /**
     * Returns modules that configure application specific behaviour that is not
     * related to the servlet configuration.
     * 
     * @return an array of modules
     */
    protected abstract List<Module> getJoynrModules();
}
