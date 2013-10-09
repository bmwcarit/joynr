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
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.UUID;

import org.eclipse.xtend.core.XtendInjectorSingleton;
import org.eclipse.xtend.core.compiler.batch.XtendBatchCompiler;

import com.google.inject.Injector;

public class TemplatesLoader {

    private static XtendBatchCompiler xtendCompiler;

    private static OnTheFlyJavaCompiler javaCompiler;

    static {
        Injector injector = XtendInjectorSingleton.INJECTOR;
        xtendCompiler = injector.getInstance(XtendBatchCompiler.class);
        javaCompiler = injector.getInstance(OnTheFlyJavaCompiler.class);
    }

    public static void load(String templatePath, String encoding) throws IOException {
        final File sysTempDir = new File(System.getProperty("java.io.tmpdir"));
        String outputPath = sysTempDir.getAbsolutePath() + File.separator + UUID.randomUUID().toString();
        xtendCompiler.setOutputPath(outputPath);
        //		compiler.setClassPath(arguments.next().trim());
        //		compiler.setTempDirectory(arguments.next().trim());
        xtendCompiler.setFileEncoding(encoding);
        xtendCompiler.setUseCurrentClassLoaderAsParent(true);
        xtendCompiler.setSourcePath(templatePath);
        //		compiler.setUseCurrentClassLoaderAsParent(true);
        xtendCompiler.compile();

        //compileXtendFiles(filter(modelsIn(folder(templatePath)), XtendFile.class), outputPath);
        compileJavaClasses(outputPath);
        updateClassloader(outputPath);
    }

    private static void compileJavaClasses(String sourcePath) {
        javaCompiler.setParentClassLoader(Thread.currentThread().getContextClassLoader());
        javaCompiler.compileClassesInSourcePath(sourcePath);
    }

    private static void updateClassloader(String outputPath) throws MalformedURLException {
        Thread currentThread = Thread.currentThread();
        URLClassLoader templateCL = URLClassLoader.newInstance(new URL[]{ new URL("file://" + outputPath
                + File.separator) }, currentThread.getContextClassLoader());
        currentThread.setContextClassLoader(templateCL);
    }
}
