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

import static java.util.Arrays.asList;

import java.io.File;
import java.io.FileFilter;
import java.io.FilenameFilter;
import java.util.Set;

import org.eclipse.emf.common.util.URI;

import com.google.common.base.Function;
import com.google.common.collect.Iterables;

public class FolderUriProvider implements IUriProvider {

    private final File folder;

    private final Set<String> allowedExtension;

    public static IUriProvider folder(Set<String> allowedExtension, String folder) {
        return new FolderUriProvider(allowedExtension, new File(folder));
    }

    public FolderUriProvider(Set<String> allowedExtension, File folder) throws IllegalArgumentException {
        if (!folder.exists()) {
            throw new IllegalArgumentException(folder + " does not exist");
        }
        this.folder = folder;
        this.allowedExtension = allowedExtension;
    }

    public Iterable<URI> allUris() {
        return getFilenamesOfFolder(folder);
    }

    private Iterable<URI> getFilenamesOfFolder(final File myFolder) {
        Iterable<URI> result = getFilesUris(myFolder);

        // subfolder
        for (File subfolder : getSubfolders(myFolder)) {
            result = Iterables.concat(getFilenamesOfFolder(subfolder), result);
        }
        return result;
    }

    private Iterable<URI> getFilesUris(final File folder) {
        String[] filenames = folder.list(new FilenameFilter() {
            public boolean accept(File dir, String name) {
                int extPosition = name.lastIndexOf('.');
                String extension = name.substring(extPosition + 1);
                return allowedExtension.contains(extension);
            }
        });
        return Iterables.transform(asList(filenames), new Function<String, URI>() {
            public URI apply(String from) {
                return URI.createFileURI(new File(folder + File.separator + from).getAbsolutePath());
            }
        });
    }

    private File[] getSubfolders(File folder) {
        File[] subfolders = folder.listFiles(new FileFilter() {
            public boolean accept(File file) {
                return file.isDirectory();
            }
        });
        return subfolders;
    }

}
