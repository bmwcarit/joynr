package io.joynr.generator.util;

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

import static org.eclipse.xtext.util.Files.cleanFolder;

import java.io.File;
import java.io.FileNotFoundException;

public class GenerationBase {
    protected void createFolder(String path) {
        File dir = new File(path);
        if (!dir.exists()) {
            dir.mkdirs();
        }
    }

    protected void cleanDirectory(String path) {
        File directory = new File(path);
        if (!directory.exists()) {
            directory.mkdirs();
        } else {
            try {
                cleanFolder(directory, new IgnoreSVNFileFilter(), true, false);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
        }

    }
}
