/*
 * #%L
 * %%
 * Copyright (C) 2024 BMW Car IT GmbH
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

import org.jboss.shrinkwrap.resolver.api.Resolvers;
import org.jboss.shrinkwrap.resolver.api.maven.MavenResolverSystem;

import java.io.File;

public class MavenDependencyHelper {

    /**
     * Scans pom file and queries all test dependencies from it
     * @return array of dependencies
     */
    public static File[] getMavenDependencies() {
        return Resolvers.use(MavenResolverSystem.class)
                        .loadPomFromFile("pom.xml")
                        .importTestDependencies()
                        .resolve()
                        .withTransitivity()
                        .as(File.class);
    }
}
