package io.joynr.generator.util;

/*
 * #%L
 * joynr::tools::generator::joynr Generator Framework2
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

import org.eclipse.xtext.util.Modules2;
import org.franca.core.dsl.FrancaIDLRuntimeModule;
import org.franca.core.dsl.FrancaIDLStandaloneSetup;

import com.google.inject.Guice;
import com.google.inject.Injector;

public class FrancaIDLFrameworkStandaloneSetup extends FrancaIDLStandaloneSetup {
    @Override
    public Injector createInjector() {
        return Guice.createInjector(Modules2.mixin(new FrancaIDLRuntimeModule(), new FrancaIDLFrameworkModule()));
    }

    @Override
    public Injector createInjectorAndDoEMFRegistration() {
        //        EPackage.Registry.INSTANCE.put(GenModelPackage.eNS_URI, GenModelPackage.eINSTANCE);
        return super.createInjectorAndDoEMFRegistration();
    }
}
