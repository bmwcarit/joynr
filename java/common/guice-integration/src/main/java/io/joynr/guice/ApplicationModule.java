package io.joynr.guice;

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

import java.util.Properties;

import com.google.inject.name.Names;

/**
 * This class is used to configure the Guice InjectorFactory when creating applications.
 * This module binds the unique identifier of the application as well as the subclass of IApplication
 * which is binded for instantiaton.
 */
public class ApplicationModule extends PropertyLoadingModule {

    private String fAppId = null;
    protected Class<? extends IApplication> fApplicationClass;

    /**
     * @param applicationClass the class used for application instantiation
     */
    public ApplicationModule(Class<? extends IApplication> applicationClass) {
        this(applicationClass.getName(), applicationClass);
    }

    /**
     * @param appId the unique identified of the applicaiton to be generated
     * @param applicationClass the class used for application instantiation
     */
    public ApplicationModule(String appId, Class<? extends IApplication> applicationClass) {
        this(appId, applicationClass, new Properties());
    }

    /**
     * @param appId the unique identified of the applicaiton to be generated
     * @param applicationClass the class used for application instantiation
     * @param properties application specific properties to be binded via this module
     */
    public ApplicationModule(String appId, Class<? extends IApplication> applicationClass, Properties properties) {
        super(properties == null ? new Properties() : properties);
        this.fAppId = appId;
        this.fApplicationClass = applicationClass;
    }

    @Override
    protected void configure() {
        super.configure();
        bindApplication();
    }

    protected void bindApplication() {
        bind(String.class).annotatedWith(Names.named(IApplication.APPLICATION_ID)).toInstance(fAppId);
        bind(IApplication.class).to(fApplicationClass);
    }

    protected String getAppId() {
        return fAppId;
    }
}
