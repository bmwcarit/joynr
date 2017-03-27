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

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;

/**
 * @param <T> the kind of application module expected by the InjectorFactory. The application module provides the required information of the runtime environment
 * @param <S> the kind of application which shall be created by this InjectorFactoryF
 */
public class InjectorFactory<T extends ApplicationModule, S extends IApplication> {
    //    protected Logger logger = Logger.getLogger(getClass());
    private Injector fInjector;
    private Module[] modules;
    private Injector rootInjector;

    /**
     * @param modules a list of modules to be used for the injector creation
     *
     * Basic constructor for the Injectorfactory
     */
    public InjectorFactory(Module... modules) {
        this.modules = modules;
    }

    /**
     * @param upateModules the updateModules which replace the previously defined modules of the InjectorFactory
     */
    public void updateModules(Module... upateModules) {
        if (fInjector == null) {
            this.modules = upateModules;
        } else {
            throw new RuntimeException("updateModules shall be invoked BEFORE the factory gets used for the first time!");
        }
    }

    protected Module[] getModules() {
        return modules;
    }

    /**
     * @param applicationModule the mandatory application properties binded via this module
     * @param applicationSpecificModules modules allowing to further configure the application specific injector used for the creation of the application
     * @return a new instance of {@literal <S extends IApplication>}
     */
    @SuppressWarnings("unchecked")
    public S createApplication(T applicationModule, Module... applicationSpecificModules) {
        //        Module applicationModule = activator.getApplicationSpecificModule(appId);
        Module[] modules = new Module[applicationSpecificModules != null ? applicationSpecificModules.length + 1 : 1];
        modules[0] = applicationModule;
        if (applicationSpecificModules != null) {
            for (int i = 0; i < applicationSpecificModules.length; i++) {
                modules[i + 1] = applicationSpecificModules[i];
            }
        }

        return (S) createChildInjector(modules).getInstance(IApplication.class);
    }

    /**
     * @return a new instance of {@literal <S extends IApplication>}
     *
     * Create a new instance of {@literal <S extends IApplcation>},
     * based on the default injector configurations of the InjectorFactory
     */
    @SuppressWarnings("unchecked")
    public S createApplication() {
        return (S) (createChildInjector().getInstance(IApplication.class));
    }

    /**
     * @param rootInjector the new rootInjector
     *
     * Allows to adapt the root injector of the InjectorFactory
     */
    protected void setRootInjector(Injector rootInjector) {
        if (fInjector == null) {
            this.rootInjector = rootInjector;
        } else {
            throw new RuntimeException("setRootInjector shall be invoked BEFORE the factory gets used for the first time!");
        }
    }

    /**
     * @return the injector created by the InjectorFactory based on the configured modules
     */
    public Injector getInjector() {
        if (fInjector == null) {
            fInjector = rootInjector == null ? Guice.createInjector(modules)
                    : rootInjector.createChildInjector(modules);
        }
        return fInjector;
    }

    /**
     * @param withModules modules used for the creation of the child injector
     * @return a new injector which has been derived from the "main" injector of the InjectorFactory
     */
    public Injector createChildInjector(Module... withModules) {
        return getInjector().createChildInjector(withModules);
    }
}
