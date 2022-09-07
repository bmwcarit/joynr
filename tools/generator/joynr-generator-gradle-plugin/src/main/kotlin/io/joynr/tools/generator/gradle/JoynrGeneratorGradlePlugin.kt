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

import com.android.build.gradle.BaseExtension
import com.android.build.gradle.LibraryExtension
import com.android.build.gradle.internal.dsl.BaseAppModuleExtension
import org.gradle.api.JavaVersion
import org.gradle.api.Plugin
import org.gradle.api.Project
import java.io.File

class JoynrGeneratorGradlePlugin : Plugin<Project> {

    companion object {
        const val JOYNR_GENERATOR_EXTENSION_NAME = "joynrGenerator"
        const val JOYNR_GENERATE_TASK_NAME = "joynrGenerate"
        const val CLEAN_TASK_NAME = "clean"
        private const val COMPILE_TASK = "compile"
        const val TASK_NAME_MATCHER_STRING = "^$COMPILE_TASK\\w*"
        private const val ANDROID_DEFAULT_OUTPUT_PATH_APP = "app/build"
        private const val ANDROID_DEFAULT_OUTPUT_PATH_DIRECT = "build"
        private const val ANDROID_DEFAULT_OUTPUT_PATH_GEN = "generated/source/fidl/"
        private const val JOYNR_ANDROID_PACKAGING_OPTIONS_MERGE_PATTERN_DEPENDENCIES    = "/META-INF/DEPENDENCIES"
        private const val JOYNR_ANDROID_PACKAGING_OPTIONS_MERGE_PATTERN_INDEX_LIST      = "/META-INF/INDEX.LIST"
        private const val JOYNR_ANDROID_PACKAGING_OPTIONS_MERGE_PATTERN_NETTY_VERSIONS  = "/META-INF/io.netty.versions.properties"
    }

    override fun apply(project: Project) {
        val extension = project.extensions.create(
            JOYNR_GENERATOR_EXTENSION_NAME,
            JoynrGeneratorPluginExtension::class.java,
            project
        )

        if (extension.outputPath.orNull == null) {
            val outputDir =
                File("${project.projectDir.absolutePath}/$ANDROID_DEFAULT_OUTPUT_PATH_APP")
            if (!outputDir.exists() || !outputDir.isDirectory) {
                extension.outputPath.set(
                    "${project.projectDir
                        .absolutePath}/$ANDROID_DEFAULT_OUTPUT_PATH_DIRECT/$ANDROID_DEFAULT_OUTPUT_PATH_GEN"
                )
            } else {
                extension.outputPath.set(
                    "${project.projectDir
                        .absolutePath}/$ANDROID_DEFAULT_OUTPUT_PATH_APP/$ANDROID_DEFAULT_OUTPUT_PATH_GEN"
                )
            }
        }

        val joynrGeneratorArgumentHandler = JoynrGeneratorArgumentHandler(
            project.logger,
            extension.modelPath, extension.outputPath, extension.generationLanguage,
            extension.generationId, extension.target, extension.skip, extension.addVersionTo,
            extension.extraParameters, project
        )

        val task = project.tasks.create(
            JOYNR_GENERATE_TASK_NAME,
            JoynrGeneratorTask::class.java
        ) { generatorTask ->
            generatorTask.joynrGeneratorHandler =
                JoynrGeneratorHandler(project.logger, joynrGeneratorArgumentHandler)
        }

        project.tasks.findByName(CLEAN_TASK_NAME)?.let {
            it.doLast {
                joynrGeneratorArgumentHandler.setClean(true)
                val generatorHandler =
                    JoynrGeneratorHandler(project.logger, joynrGeneratorArgumentHandler)
                generatorHandler.execute()
                joynrGeneratorArgumentHandler.setClean(false)
            }
        }

        project.afterEvaluate {
            project.tasks.forEach {
                when {
                    it.name.matches(TASK_NAME_MATCHER_STRING.toRegex()) -> {
                        // add joynr generate task as a dependency for the appropriate tasks, so we
                        // automatically generate code when developers build project with a correctly
                        // configured joynr generator
                        it.dependsOn(project.tasks.getByName(JOYNR_GENERATE_TASK_NAME))
                    }

                    else -> {
                        // do nothing
                    }
                }
            }

        }

        // Register our task with the variant's sources
        val outputDir: File?
        outputDir = if (extension.outputPath.orNull == null) {
            File("${project.rootDir}/${extension.outputPath.get()}")
        } else {
            File(extension.outputPath.get())
        }

        project.extensions.findByName("android")?.run {
            val androidComponentExtension = project.properties["android"]

            if (androidComponentExtension is BaseExtension) {
                androidComponentExtension.sourceSets.getByName("main").java.srcDirs(outputDir)

                // merge /META-INF DEPENDENCIES, INDEX.LIST and io.netty.versions.properties
                // to avoid conflicts during assemble
                androidComponentExtension.packagingOptions.merge(
                        JOYNR_ANDROID_PACKAGING_OPTIONS_MERGE_PATTERN_DEPENDENCIES
                )
                androidComponentExtension.packagingOptions.merge(
                        JOYNR_ANDROID_PACKAGING_OPTIONS_MERGE_PATTERN_INDEX_LIST
                )
                androidComponentExtension.packagingOptions.merge(
                        JOYNR_ANDROID_PACKAGING_OPTIONS_MERGE_PATTERN_NETTY_VERSIONS
                )

                androidComponentExtension.compileOptions.sourceCompatibility =
                    JavaVersion.VERSION_1_8
                androidComponentExtension.compileOptions.targetCompatibility =
                    JavaVersion.VERSION_1_8
            }

            if (androidComponentExtension is BaseAppModuleExtension) {
                androidComponentExtension.applicationVariants.all { variant ->
                    variant.registerJavaGeneratingTask(task, outputDir)
                }
            } else if (androidComponentExtension is LibraryExtension) {
                androidComponentExtension.libraryVariants.all { variant ->
                    variant.registerJavaGeneratingTask(task, outputDir)
                }
            }
        }

    }

}
