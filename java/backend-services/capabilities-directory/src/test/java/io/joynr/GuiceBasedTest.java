package io.joynr;

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

import static com.google.common.collect.Iterables.toArray;
import static com.google.inject.Guice.createInjector;
import static com.google.inject.util.Modules.combine;

import org.junit.Before;

import com.google.inject.Injector;
import com.google.inject.Module;

public abstract class GuiceBasedTest {
    private static Injector injector = null;
    private static Class<?> lastType = null;

    @Before
    public void setUp() throws Exception {
        injectMembers();
    }

    public abstract Iterable<Module> getModules();

    protected void injectMembers() {
        getInjector().injectMembers(this);
    }

    protected Injector getInjector() {
        if (lastType != getClass() || injector == null) {
            injector = createInjector(combine(toArray(getModules(), Module.class)));
            lastType = getClass();
        }
        return injector;
    }
}
