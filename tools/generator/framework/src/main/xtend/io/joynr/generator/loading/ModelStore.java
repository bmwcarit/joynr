package io.joynr.generator.loading;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static java.util.Arrays.asList;
import static java.util.Collections.emptyMap;

import java.io.IOException;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.runtime.Assert;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.util.EcoreUtil;
import org.eclipse.xtext.resource.XtextResource;
import org.eclipse.xtext.resource.XtextResourceSet;
import org.eclipse.xtext.util.StringInputStream;
import org.franca.core.dsl.FrancaIDLStandaloneSetup;

import com.google.common.base.Joiner;
import com.google.common.collect.Iterators;
import com.google.inject.Inject;
import com.google.inject.Injector;

public class ModelStore implements Iterable<EObject> {

    private XtextResourceSet resourceSet;
    {
        Injector injector = new FrancaIDLStandaloneSetup().createInjectorAndDoEMFRegistration();
        resourceSet = injector.getInstance(XtextResourceSet.class);
        resourceSet.addLoadOption(XtextResource.OPTION_RESOLVE_ALL, Boolean.TRUE);
    }
    private IUriProvider uriProvider;

    @Inject
    public ModelStore(IUriProvider uriProvider) {
        super();
        this.uriProvider = uriProvider;
        //		resourceSet.setClasspathURIContext(getClass().getClassLoader());
    }

    public Resource getResource(URI uri) {
        return resourceSet.getResource(uri, true);
    }

    public List<Resource> load() {
        for (URI uri : uriProvider.allUris()) {
            try {
                //            	System.out.println("resource: " + uri.path());
                Resource resource = resourceSet.getResource(uri, false);
                if (resource == null) {
                    //            		System.out.println("- create");
                    resource = resourceSet.createResource(uri);
                }
                resource.load(null);
                //            	System.out.println("- load");
                EcoreUtil.resolveAll(resource);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        EcoreUtil.resolveAll(resourceSet);
        return resourceSet.getResources();
    }

    public void add(Resource... resources) {
        resourceSet.getResources().addAll(asList(resources));
    }

    public Resource parse(String name, String[] lines) {
        Resource resource = createResource(name);
        String inputString = Joiner.on("\n").join("\n", lines);
        StringInputStream inputStream = new StringInputStream(inputString);
        try {
            resource.load(inputStream, emptyMap());
        } catch (IOException ex) {
            ex.printStackTrace();
            Assert.isTrue(false, ex.getMessage());
        }
        return resource;
    }

    private Resource createResource(String name) {
        URI uri = URI.createURI(name);
        Resource resource = resourceSet.getResource(uri, false);
        if (resource != null) {
            resource.unload();
        } else {
            resource = resourceSet.createResource(uri);
        }
        return resource;
    }

    public Iterator<EObject> iterator() {
        return Iterators.filter(resourceSet.getAllContents(), EObject.class);
    }

    public static ModelStore modelsIn(IUriProvider uriProvider) {
        ModelStore modelStore = new ModelStore(uriProvider);
        //		if (uriProvider instanceof ClassPathUriProvider){
        //			modelStore.resourceSet.setClasspathURIContext(Thread.currentThread().getContextClassLoader());
        //		}
        modelStore.load();
        return modelStore;
    }

}
