/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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


import org.gradle.api.Plugin
import org.gradle.api.Project

class JoynrGeneratorGradlePlugin : Plugin<Project> {

    override fun apply(project: Project) {
        val extension = project.extensions.create("joynrGenerator", JoynrGeneratorPluginExtension::class.java, project)
        val joynrGeneratorArgumentHandler = JoynrGeneratorArgumentHandler(project.logger,
                extension.modelPath, extension.outputPath, extension.generationLanguage,
                extension.rootGenerator, extension.generationId, extension.skip,
                extension.addVersionTo, extension.extraParameters)
        project.tasks.create("joynr-generate", JoynrGeneratorTask::class.java) { generatorTask ->
            generatorTask.joynrGeneratorHandler = JoynrGeneratorHandler(project.logger, joynrGeneratorArgumentHandler)
        }
        project.tasks.getByName("clean").doLast {
            joynrGeneratorArgumentHandler.setClean(true)
            val generatorHandler = JoynrGeneratorHandler(project.logger, joynrGeneratorArgumentHandler)
            generatorHandler.execute()
        }
    }
}