/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.tools.generator.gradle

import org.gradle.api.Project

/**
 * All variables defined in this class can be configured
 * in the gradle build script using this plugin.
 */
open class JoynrGeneratorPluginExtension(project: Project) {
    /**
     * Path to model file or directory containing multiple modelPath files.
     * This is a required parameter.
     */
    val modelPath = project.objects.property(String::class.java)!!

    /**
     * Path where the generated source code is to be placed.
     * This is a required parameter.
     */
    val outputPath = project.objects.property(String::class.java)!!

    /**
     * Sets the generation language. This is a required parameter.
     */
    val generationLanguage = project.objects.property(String::class.java)!!

    /**
     * Sets the generation ID (optional).
     */
    val generationId = project.objects.property(String::class.java)!!

    /**
     * Sets the target (optional).
     */
    val target = project.objects.property(String::class.java)!!

    /**
     * If set to true, execution of the joynr generator is skipped (optional).
     */
    val skip = project.objects.property(java.lang.Boolean::class.java)!!

    /**
     * Specify how the major version shall affect the generated interface name and package.
     */
    val addVersionTo = project.objects.property(String::class.java)!!

    /**
     * Sets extra parameters that may be required by custom generators (optional).
     * In the gradle build file, this has to be set using a Map<String, String> structure.
     */
    val extraParameters = project.objects.mapProperty(String::class.java, String::class.java)!!
}
