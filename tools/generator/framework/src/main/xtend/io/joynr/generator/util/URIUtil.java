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

import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

public class URIUtil {
    //    private static final String JAR_SUFFIX = "!/"; //$NON-NLS-1$
    private static final String UNC_PREFIX = "//"; //$NON-NLS-1$
    private static final String SCHEME_FILE = "file"; //$NON-NLS-1$

    //    private static final String SCHEME_JAR = "jar"; //$NON-NLS-1$

    public static URI toURI(URL url) throws URISyntaxException {
        //URL behaves differently across platforms so for file: URLs we parse from string form
        if (SCHEME_FILE.equals(url.getProtocol())) {
            String pathString = url.toExternalForm().substring(5);
            //ensure there is a leading slash to handle common malformed URLs such as file:c:/tmp
            if (pathString.indexOf('/') != 0)
                pathString = '/' + pathString;
            else if (pathString.startsWith(UNC_PREFIX) && !pathString.startsWith(UNC_PREFIX, 2)) {
                //URL encodes UNC path with two slashes, but URI uses four (see bug 207103)
                pathString = ensureUNCPath(pathString);
            }
            return new URI(SCHEME_FILE, null, pathString, null);
        }
        try {
            return new URI(url.toExternalForm());
        } catch (URISyntaxException e) {
            //try multi-argument URI constructor to perform encoding
            return new URI(url.getProtocol(),
                           url.getUserInfo(),
                           url.getHost(),
                           url.getPort(),
                           url.getPath(),
                           url.getQuery(),
                           url.getRef());
        }
    }

    private static String ensureUNCPath(String path) {
        int len = path.length();
        StringBuffer result = new StringBuffer(len);
        for (int i = 0; i < 4; i++) {
            //	if we have hit the first non-slash character, add another leading slash
            if (i >= len || result.length() > 0 || path.charAt(i) != '/')
                result.append('/');
        }
        result.append(path);
        return result.toString();
    }
}
