/*
 * #%L
 * %%
 * Copyright (C) 2025 BMW Car IT GmbH
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
package itest.io.joynr.jeeintegration.base.deployment;

import java.io.File;

public class FileConstants {

    private static final String BEANS_XML = "src/main/resources/META-INF/beans.xml";
    private static final String EXTENSION = "src/main/resources/META-INF/services/jakarta.enterprise.inject.spi.Extension";

    /**
     * There is no need to instantiate class with static fields/methods only
     */
    private FileConstants() {

    }

    private static File getFile(final String filePath) {
        return new File(filePath);
    }

    public static File getBeansXml() {
        return getFile(BEANS_XML);
    }

    public static File getExtension() {
        return getFile(EXTENSION);
    }
}
