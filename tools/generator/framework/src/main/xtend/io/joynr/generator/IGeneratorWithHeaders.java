package io.joynr.generator;

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
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.IFileSystemAccess;

/**
 * Interface for Generators that support generating header files to a separate directory
 */
public interface IGeneratorWithHeaders extends IJoynrGenerator {

    /**
     * @param input - the input for which to generate resources
     * @param sourceFileSystem - file system access to be used to generate source files
     * @param headerFileSystem - file system access to be used to generate header files
     */
    public void doGenerate(Resource input, IFileSystemAccess sourceFileSystem, IFileSystemAccess headerFileSystem);
}
