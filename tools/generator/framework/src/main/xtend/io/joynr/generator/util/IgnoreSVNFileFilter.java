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

import java.io.File;
import java.io.FileFilter;

public class IgnoreSVNFileFilter implements FileFilter {

    public IgnoreSVNFileFilter() {

    }

    public boolean accept(File file) {
        return (!file.getAbsolutePath().contains(".svn"));
    }

}
