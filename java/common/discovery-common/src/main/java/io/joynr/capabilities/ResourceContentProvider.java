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
package io.joynr.capabilities;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ResourceContentProvider {

    private static final Logger logger = LoggerFactory.getLogger(ResourceContentProvider.class);

    private static final Charset UTF8 = Charset.forName("UTF-8");

    public String readFromFileOrResourceOrUrl(String provisionedCapabilitiesJsonFilename) {
        logger.trace("Attempting to read statically provisioned capabilities from JSON in file/resource/URL: {}",
                     provisionedCapabilitiesJsonFilename);
        IOException ioException = null;
        String result = null;
        try {
            URI uri = new URI(provisionedCapabilitiesJsonFilename);
            if (!uri.isAbsolute()) {
                throw new URISyntaxException(provisionedCapabilitiesJsonFilename, "URI is not absolute");
            }
            result = readFromUri(uri);
        } catch (URISyntaxException e) {
            logger.trace("{} is not a URL. Trying to read from filesystem/classpath.",
                         provisionedCapabilitiesJsonFilename,
                         e);
        } catch (IOException e) {
            ioException = e;
        }
        if (result == null && ioException == null) {
            try {
                result = readFromFileOrClasspath(provisionedCapabilitiesJsonFilename);
            } catch (IOException e) {
                ioException = e;
            }
        }
        if (ioException != null) {
            throw new IllegalArgumentException("Unable to read provisioned capabilities from "
                    + provisionedCapabilitiesJsonFilename, ioException);
        }
        if (result == null) {
            throw new IllegalStateException(provisionedCapabilitiesJsonFilename
                    + " not found as URL or on filesystem or classpath.");
        }
        return result;
    }

    private String readFromFileOrClasspath(String provisionedCapabilitiesJsonFilename) throws IOException {
        logger.trace("Attempting to read {} from file / classpath", provisionedCapabilitiesJsonFilename);
        Path filePath = Paths.get(provisionedCapabilitiesJsonFilename);
        if (Files.exists(filePath)) {
            return new String(Files.readAllBytes(filePath), "UTF-8");
        } else {
            logger.trace("File {} doesn't exist on filesystem, attempting to read from classpath.",
                         provisionedCapabilitiesJsonFilename);
            try (InputStream resourceAsStream = StaticCapabilitiesProvisioning.class.getClassLoader()
                                                                                    .getResourceAsStream(provisionedCapabilitiesJsonFilename)) {
                if (resourceAsStream != null) {
                    return readFromStream(resourceAsStream);
                }
            }
        }
        return null;
    }

    private String readFromUri(URI uri) throws IOException {
        URL url = new URL(uri.toString());
        logger.trace("Attempting to read from URL {}", url);
        InputStream inputStream = url.openStream();
        return readFromStream(inputStream);
    }

    private String readFromStream(InputStream inputStream) throws IOException {
        try (InputStreamReader reader = new InputStreamReader(inputStream, UTF8);
                BufferedReader bufferedReader = new BufferedReader(reader)) {
            StringBuilder builder = new StringBuilder();
            String line = null;
            while ((line = bufferedReader.readLine()) != null) {
                builder.append(line);
            }
            return builder.toString();
        } catch (IOException e) {
            throw e;
        }
    }
}
