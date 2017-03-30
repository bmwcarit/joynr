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

import java.io.File;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Collections;

import org.apache.log4j.Logger;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;

import com.google.common.collect.Lists;
import com.google.common.collect.Sets;

public class ModelLoader {

    private final IUriProvider uriProvider;
    private final ModelStore modelStore;
    private static Logger logger = Logger.getLogger(ModelLoader.class);

    public ModelLoader(final String modelpath) {
        File modelFile = new File(modelpath);
        if (modelFile.exists()) {

            File file = modelFile;

            if (file.isDirectory()) {
                uriProvider = new FolderUriProvider(Sets.newHashSet("fidl"), file);
            } else {
                final URI uri = URI.createFileURI(modelFile.getAbsolutePath());
                uriProvider = new IUriProvider() {

                    @Override
                    public Iterable<URI> allUris() {
                        return Lists.newArrayList(uri);
                    }
                };
            }
        } else {
            uriProvider = new IUriProvider() {

                @Override
                public Iterable<URI> allUris() {
                    URL resource = getClass().getClassLoader().getResource(modelpath);
                    if (resource != null) {
                        try {
                            return Lists.newArrayList(URI.createURI(resource.toURI().toString()));
                        } catch (URISyntaxException e) {
                            logger.error("An error occurred while attempting to convert a java.net.URI to an emf URI.",
                                         e);
                        }
                    }
                    return Collections.emptyList();
                }
            };
        }

        modelStore = new ModelStore(uriProvider);

    }

    public Iterable<Resource> getResources() {
        return modelStore.getResources();
    }

}
