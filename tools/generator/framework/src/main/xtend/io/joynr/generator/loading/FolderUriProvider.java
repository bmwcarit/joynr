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

import static java.util.Arrays.asList;

import java.io.File;
import java.io.FileFilter;
import java.io.FilenameFilter;
import java.util.Set;

import org.eclipse.emf.common.util.URI;

import com.google.common.base.Function;
import com.google.common.collect.Iterables;
import com.google.common.collect.Sets;

public class FolderUriProvider implements IUriProvider {

    private static final String SEPARATOR = "/";

    private final File folder;

    private Set<String> allowedExtension = Sets.newHashSet("xml", "xtend");

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
        File[] subfolders = getSubfolders(myFolder);
        for (File subfolder : subfolders) {
            result = Iterables.concat(getFilenamesOfFolder(subfolder), result);
        }
        return result;
    }

    private Iterable<URI> getFilesUris(final File myFolder) {
        String[] filenames = myFolder.list(new FilenameFilter() {
            public boolean accept(File dir, String name) {
                int extPosition = name.lastIndexOf('.');
                String extension = name.substring(extPosition + 1);
                return allowedExtension.contains(extension);
            }
        });
        return Iterables.transform(asList(filenames), new Function<String, URI>() {
            public URI apply(String from) {
                return URI.createFileURI(new File(myFolder + SEPARATOR + from).getAbsolutePath());
            }
        });
    }

    private File[] getSubfolders(File myFolder) {
        File[] subfolders = myFolder.listFiles(new FileFilter() {
            public boolean accept(File file) {
                return file.isDirectory();
            }
        });
        return subfolders;
    }

}
