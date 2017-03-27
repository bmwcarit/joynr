package io.joynr.generator.loading;

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

import java.io.IOException;
import java.util.Iterator;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.util.EcoreUtil;
import org.eclipse.xtext.resource.XtextResource;
import org.eclipse.xtext.resource.XtextResourceSet;
import org.franca.core.dsl.FrancaIDLStandaloneSetup;

import com.google.common.collect.Iterators;
import com.google.inject.Inject;
import com.google.inject.Injector;

public class ModelStore implements Iterable<EObject> {

    private final XtextResourceSet resourceSet;
    {
        Injector injector = new FrancaIDLStandaloneSetup().createInjectorAndDoEMFRegistration();
        resourceSet = injector.getInstance(XtextResourceSet.class);
        resourceSet.addLoadOption(XtextResource.OPTION_RESOLVE_ALL, Boolean.TRUE);
    }

    @Inject
    public ModelStore(IUriProvider uriProvider) {
        super();
        load(uriProvider);
    }

    private void load(IUriProvider uriProvider) {
        for (URI uri : uriProvider.allUris()) {
            try {
                Resource resource = resourceSet.getResource(uri, false);
                if (resource == null) {
                    resource = resourceSet.createResource(uri);
                }
                resource.load(null);
                EcoreUtil.resolveAll(resource);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        EcoreUtil.resolveAll(resourceSet);
    }

    public Iterator<EObject> iterator() {
        return Iterators.filter(resourceSet.getAllContents(), EObject.class);
    }

    public Iterable<Resource> getResources() {
        return resourceSet.getResources();
    }
}
