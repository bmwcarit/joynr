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

import io.joynr.loading.FolderUriProvider;
import io.joynr.loading.IUriProvider;
import io.joynr.loading.ModelStore;

import java.io.File;
import java.util.HashSet;
import java.util.Set;

import javax.inject.Inject;

import junit.framework.TestCase;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;
import org.eclipse.xtext.generator.AbstractFileSystemAccess;
import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.junit4.InjectWith;
import org.eclipse.xtext.junit4.XtextRunner;
import org.franca.core.dsl.FrancaIDLTestsInjectorProvider;
import org.junit.runner.RunWith;

import com.google.common.collect.Lists;
import com.google.common.collect.Sets;
import com.google.inject.Binder;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;

@RunWith(XtextRunner.class)
@InjectWith(FrancaIDLTestsInjectorProvider.class)
public abstract class AbstractGeneratorTest extends TestCase {

    private Injector injector = null;

    ResourceSet resourceSet = new ResourceSetImpl();
    @Inject
    private IFileSystemAccess fileSystemAccess;

    protected Injector getInjector() {
        if (injector == null)
            injector = Guice.createInjector(new Module() {
                @Override
                public void configure(final Binder binder) {
                    binder.bind(ResourceSet.class).to(ResourceSetImpl.class);
                    bindGeneratorClass(binder);
                }
            });

        return injector;
    }

    protected abstract void bindGeneratorClass(final Binder binder);

    protected void invokeGenerator(IGenerator generator,
                                   String fileName,
                                   String outputDirectory,
                                   String... referencedResources) {
        final IFileSystemAccess fileSystemAccess = createFileSystemAccess(outputDirectory);

        final URI uri = URI.createFileURI(new File(fileName).getAbsolutePath());
        final Set<URI> uris = new HashSet<URI>();
        uris.add(uri);
        for (String refRes : referencedResources) {
            uris.add(URI.createFileURI(new File(refRes).getAbsolutePath()));
        }
        File file = new File(fileName);
        IUriProvider uriProvider = null;
        if (file.isDirectory()) {
            uriProvider = new FolderUriProvider(Sets.newHashSet("fidl"), file);
        } else {
            uriProvider = new IUriProvider() {

                @Override
                public Iterable<URI> allUris() {
                    return Lists.newArrayList(uris);
                }
            };
        }

        ModelStore modelStore = ModelStore.modelsIn(uriProvider);

        for (URI foundUri : uriProvider.allUris()) {
            final Resource r = modelStore.getResource(foundUri);
            generator.doGenerate(r, fileSystemAccess);
        }

    }

    protected IFileSystemAccess createFileSystemAccess(String outputDirectory) {

        assertTrue(fileSystemAccess instanceof AbstractFileSystemAccess);
        ((AbstractFileSystemAccess) fileSystemAccess).setOutputPath(outputDirectory);
        ((AbstractFileSystemAccess) fileSystemAccess).getOutputConfigurations()
                                                     .get(IFileSystemAccess.DEFAULT_OUTPUT)
                                                     .setCreateOutputDirectory(true);
        return fileSystemAccess;
    }

}
