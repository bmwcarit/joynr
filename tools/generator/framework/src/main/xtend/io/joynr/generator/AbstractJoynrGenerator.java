package io.joynr.generator;

/*
 * #%L
 * io.joynr.tools.generator:generator-framework
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import com.google.inject.Binder;
import com.google.inject.Module;

public abstract class AbstractJoynrGenerator implements IJoynrGenerator {

    @Override
    public Module getGeneratorModule() {
        return new Module() {
            @Override
            public void configure(Binder binder) {
            }
        };
    }

}
