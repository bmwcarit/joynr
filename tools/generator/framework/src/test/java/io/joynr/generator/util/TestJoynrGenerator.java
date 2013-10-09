package io.joynr.generator.util;

/*
 * #%L
 * io.joynr.tools.generator:generator-framework
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

import io.joynr.generator.IJoynrGenerator;

import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.IFileSystemAccess;

public class TestJoynrGenerator implements IJoynrGenerator {

    @Override
    public void doGenerate(Resource input, IFileSystemAccess fsa) {
        // TODO Auto-generated method stub

    }

    @Override
    public String getLanguageId() {
        // TODO Auto-generated method stub
        return null;
    }

}
