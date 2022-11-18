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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import java.io.File;
import java.io.FileWriter;
import java.nio.file.Files;

import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Unit tests for the {@link ResourceContentProvider}.
 */
public class ResourceContentProviderTest {

    private Logger logger = LoggerFactory.getLogger(ResourceContentProviderTest.class);

    private File tmpFile;

    private ResourceContentProvider subject;

    @Before
    public void setup() throws Exception {
        tmpFile = Files.createTempFile("rcpt", ".txt").toFile();
        tmpFile.deleteOnExit();
        try (FileWriter writer = new FileWriter(tmpFile)) {
            writer.write("test");
            writer.flush();
        }
        subject = new ResourceContentProvider();
    }

    @Test
    public void testLoadFromFile() throws Exception {
        String result = subject.readFromFileOrResourceOrUrl(tmpFile.getAbsolutePath());
        assertNotNull(result);
        assertEquals("test", result);
    }

    @Test
    public void testLoadFromUrl() throws Exception {
        String url = tmpFile.toURI().toString();
        logger.info("Trying to load from URL: {}", url);
        String result = subject.readFromFileOrResourceOrUrl(url);
        assertNotNull(result);
        assertEquals("test", result);
    }

    @Test
    public void testLoadFromClasspath() throws Exception {
        String result = subject.readFromFileOrResourceOrUrl("testfile.txt");
        assertNotNull(result);
        assertEquals("test", result);
    }

}
