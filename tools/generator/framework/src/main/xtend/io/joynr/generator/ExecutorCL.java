package io.joynr.generator;

/*
 * #%L
 * joynr::tools::generator::joynr Generator Framework
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

public class ExecutorCL extends Executor {

    public static void main(String[] args) throws ClassNotFoundException, InstantiationException,
                                          IllegalAccessException {

        // Parse the command line arguments
        InvocationArguments invocationArguments = new InvocationArguments(args);
        if (!invocationArguments.isValid()) {
            return;
        }

        ExecutorCL executor = new ExecutorCL(invocationArguments);
        executor.setup();
        executor.execute();
    }

    public ExecutorCL(InvocationArguments arguments) {
        super(arguments);
    }

}
