package io.joynr.common;

/*
 * #%L
 * joynr::tools::generator::joynr Generator framework
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

import org.franca.core.dsl.FrancaIDLTestsInjectorProvider;

import com.google.inject.Injector;

public class TestsInjectorProvider extends FrancaIDLTestsInjectorProvider {

    @Override
    protected Injector internalCreateInjector() {
        return new TestsStandaloneSetup().createInjectorAndDoEMFRegistration();
    }
}
