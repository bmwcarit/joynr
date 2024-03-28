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
package io.joynr.common;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import com.google.inject.Inject;

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

import com.google.inject.Binder;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;

import io.joynr.loading.FolderUriProvider;
import io.joynr.loading.IUriProvider;
import io.joynr.loading.ModelStore;
import junit.framework.TestCase;

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
            uriProvider = new FolderUriProvider(new HashSet<String>(Arrays.asList("fidl")), file);
        } else {
            uriProvider = new IUriProvider() {

                @Override
                public Iterable<URI> allUris() {
                    return Arrays.asList(uris);
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
