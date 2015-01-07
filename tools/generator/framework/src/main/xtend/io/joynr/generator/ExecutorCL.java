package io.joynr.generator;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import org.eclipse.xtext.generator.IGenerator;

import io.joynr.generator.util.InvocationArguments;

public class ExecutorCL {

    public static void main(String[] args) throws ClassNotFoundException, InstantiationException,
                                          IllegalAccessException {

        // Parse the command line arguments
        InvocationArguments invocationArguments = new InvocationArguments(args);
        Executor executor = new Executor(invocationArguments);
        IGenerator generator = executor.setup();

        if (invocationArguments.isValid()) {
            executor.generate(generator);
        }
    }

}
