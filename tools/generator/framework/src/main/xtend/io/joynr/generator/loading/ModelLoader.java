package io.joynr.generator.loading;

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

import java.io.File;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import org.apache.log4j.Logger;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;

import com.google.common.collect.Lists;
import com.google.common.collect.Sets;

public class ModelLoader {

    IUriProvider uriProvider = null;
    private ModelStore modelStore = null;
    private static Logger logger = Logger.getLogger(ModelLoader.class);

    public ModelLoader(final String modelpath) {
        File modelFile = new File(modelpath);
        if (modelFile.exists()) {

            final URI uri = URI.createFileURI(modelFile.getAbsolutePath());
            final Set<URI> uris = new HashSet<URI>();
            uris.add(uri);
            File file = modelFile;

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

        modelStore = ModelStore.modelsIn(uriProvider);

        //		for (URI foundUri : uriProvider.allUris()) {
        //			final Resource r = modelStore.getResource(foundUri);
        //			generator.doGenerate(r, fileSystemAccess);
        //		}

    }

    public Iterable<URI> getURIs() {
        return uriProvider.allUris();
    }

    public Resource getResource(URI uri) {
        return modelStore.getResource(uri);
    }

}
