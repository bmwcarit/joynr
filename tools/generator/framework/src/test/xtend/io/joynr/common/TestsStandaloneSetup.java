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

import org.eclipse.emf.codegen.ecore.genmodel.GenModelPackage;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.xtext.ui.resource.IResourceSetProvider;
import org.eclipse.xtext.ui.resource.XtextResourceSetProvider;
import org.eclipse.xtext.util.Modules2;
import org.franca.core.dsl.FrancaIDLRuntimeModule;
import org.franca.core.dsl.FrancaIDLTestsModule;
import org.franca.core.dsl.FrancaIDLTestsStandaloneSetup;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;

public class TestsStandaloneSetup extends FrancaIDLTestsStandaloneSetup {
    @Override
    public Injector createInjector() {
        return Guice.createInjector(getStaticTestModule(), Modules2.mixin(new FrancaIDLRuntimeModule(),
                                                                          new FrancaIDLTestsModule()));
    }

    private Module getStaticTestModule() {
        return new AbstractModule() {

            @Override
            protected void configure() {
                bind(IResourceSetProvider.class).to(XtextResourceSetProvider.class);
            }
        };
    }

    @Override
    public Injector createInjectorAndDoEMFRegistration() {
        EPackage.Registry.INSTANCE.put(GenModelPackage.eNS_URI, GenModelPackage.eINSTANCE);
        return super.createInjectorAndDoEMFRegistration();
    }
}
