package io.joynr.generator;

/*
 * #%L
 * joynr::tools::generator::Maven joynr Generator Plugin
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * __________________
 * 
 * NOTICE:  Dissemination of this information or reproduction of this material 
 * is strictly  forbidden unless prior written permission is obtained from 
 * BMW Car IT GmbH.
 * #L%
 */

import io.joynr.generator.util.InvocationArguments;

import java.io.IOException;

public class GeneratorTask {

    InvocationArguments arguments;

    public GeneratorTask(InvocationArguments arguments) {
        this.arguments = arguments;
    }

    public void generate() throws IOException, ClassNotFoundException, InstantiationException, IllegalAccessException {
        ExecutorDEV executor = new ExecutorDEV(arguments);
        if (arguments.isValid()) {
            executor.setup();
            executor.execute();
        } else {
            throw new IllegalArgumentException(arguments.getErrorMessage());
        }
    }
}
