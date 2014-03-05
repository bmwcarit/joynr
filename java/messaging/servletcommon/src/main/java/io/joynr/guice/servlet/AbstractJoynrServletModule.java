package io.joynr.guice.servlet;

/*
 * #%L
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

import com.google.inject.servlet.GuiceFilter;
import com.sun.jersey.guice.JerseyServletModule;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

/**
 * Common base class for all joynr servlet modules.
 * 
 */
public abstract class AbstractJoynrServletModule extends JerseyServletModule {

    @Override
    protected void configureServlets() {

        // GuiceFilter is instantiated here instead of configured in
        // web.xml to allow for more than one servlet per servlet
        // container.
        // See documentation of web.xml for details.
        bind(GuiceFilter.class);

        configureJoynrServlets();

        // Route all requests through GuiceContainer
        bindJoynrServletClass();
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
     * {@code bind} for Jersey resource classes. Filters
     * can be applied by calling {@code filter}.
     */
    protected abstract void configureJoynrServlets();

}
