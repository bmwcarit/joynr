package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.Charset;

import com.google.common.io.Files;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ResourceContentProvider {

    private static final Logger logger = LoggerFactory.getLogger(ResourceContentProvider.class);

    private static final Charset UTF8 = Charset.forName("UTF-8");

    public String readFromFileOrResource(String provisionedCapabilitiesJsonFilename) {
        logger.debug("Attempting to read statically provisioned capabilities from JSON in file: {}",
                     provisionedCapabilitiesJsonFilename);
        File file = new File(provisionedCapabilitiesJsonFilename);
        IOException ioException = null;
        if (file.exists()) {
            try {
                return Files.toString(file, UTF8);
            } catch (IOException e) {
                ioException = e;
            }
        } else {
            try (InputStream resourceAsStream = StaticCapabilitiesProvisioning.class.getClassLoader()
                                                                                    .getResourceAsStream(provisionedCapabilitiesJsonFilename)) {
                if (resourceAsStream != null) {
                    try (InputStreamReader reader = new InputStreamReader(resourceAsStream, UTF8);
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
            } catch (IOException e) {
                ioException = e;
            }
        }
        if (ioException != null) {
            throw new IllegalArgumentException("Unable to read provisioned capabilities from "
                    + provisionedCapabilitiesJsonFilename, ioException);
        }
        throw new IllegalStateException(provisionedCapabilitiesJsonFilename
                + " not found in filesystem or on classpath.");
    }
}
