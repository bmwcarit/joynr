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
package io.joynr.generator.util;

import org.eclipse.xtext.generator.AbstractFileSystemAccess;
import org.eclipse.xtext.generator.IFileSystemAccess;

public class FileSystemAccessUtil {

    public static void createFileSystemAccess(IFileSystemAccess fileSystemAccess, String outputDirectory) {

        if (!(fileSystemAccess instanceof AbstractFileSystemAccess)) {
            throw new IllegalStateException("Guice Module configuration wrong: IFileSystemAccess.class shall be binded to a sub type of org.eclipse.xtext.generator.AbstractFileSystemAccess");
        }
        ((AbstractFileSystemAccess) fileSystemAccess).setOutputPath(outputDirectory);
        ((AbstractFileSystemAccess) fileSystemAccess).getOutputConfigurations()
                                                     .get(IFileSystemAccess.DEFAULT_OUTPUT)
                                                     .setCreateOutputDirectory(true);
    }

    public static String getFilePath(IFileSystemAccess fileSystemAccess, String filePath) {
        String dirPath = ((AbstractFileSystemAccess) fileSystemAccess).getOutputConfigurations()
                                                                      .get(IFileSystemAccess.DEFAULT_OUTPUT)
                                                                      .getOutputDirectory()
                                                                      .toString();
        return dirPath + "/" + filePath;
    }
}
