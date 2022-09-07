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

import io.joynr.generator.util.InvocationArguments
import org.gradle.api.Project
import org.gradle.api.logging.Logger
import org.gradle.api.provider.MapProperty
import org.gradle.api.provider.Property
import java.io.File

fun InvocationArguments.areInvocationArgumentsValid(logger: Logger): Boolean {
    try {
        checkArguments()
    } catch (e: IllegalArgumentException) {
        logger.error(e.message)
        return false
    }
    return true
}

class JoynrGeneratorArgumentHandler(
    private val logger: Logger,
    private var modelPath: Property<String>,
    private var outputPath: Property<String>,
    private var generationLanguage: Property<String>,
    private var generationId: Property<String>,
    private var target: Property<String>,
    /**
     * kotlin.Boolean can not be used as property type here,
     * as this value is set by gradle during runtime to a value of type java.lang.Boolean
     */
    var skip: Property<java.lang.Boolean>,
    var addVersionTo: Property<String>,
    var extraParameters: MapProperty<String, String>,
    private val project: Project
) {

    companion object {
        // default values for code generation
        private const val DEFAULT_LANGUAGE = "java"
        private const val ANDROID_DEFAULT_MODEL_PATH_APP = "app/src/main/fidl/"
        private const val ANDROID_DEFAULT_MODEL_PATH_SRC = "src/main/fidl/"
    }

    private var doClean: Boolean = false

    val isSkipFlagSet: Boolean
        get() = skip.isPresent && skip.get() == java.lang.Boolean(true)

    fun getGeneratorInvocationArguments(): InvocationArguments {
        val invocationArguments = InvocationArguments()
        val defaultLanguage =
            if (generationLanguage.orNull == null) DEFAULT_LANGUAGE else generationLanguage.get()
        val projectPath = project.projectDir.absolutePath

        var defaultModelPath: String?
        if (!modelPath.isPresent) {
            defaultModelPath = "$projectPath/$ANDROID_DEFAULT_MODEL_PATH_SRC"
            val modelDir = File(defaultModelPath)
            if (!modelDir.exists() || !modelDir.isDirectory) {
                defaultModelPath =
                    "$projectPath/$ANDROID_DEFAULT_MODEL_PATH_APP"
            }
        } else {
            val userModelPath = modelPath.get()
            defaultModelPath = if(userModelPath.contains(projectPath)) {
                userModelPath
            } else {
                "$projectPath/$userModelPath"
            }
            logger.quiet("$defaultModelPath")
        }
        var defaultOutputPath = ""
        if(outputPath.isPresent) {
            defaultOutputPath = if (!outputPath.get().contains(projectPath)) {
                "$projectPath/${outputPath.get()}"
            } else {
                outputPath.get()
            }
            logger.quiet("$defaultOutputPath")
        }

        invocationArguments.let {
            it.setClean(doClean)
            it.setGenerate(!doClean)
            it.modelPath = defaultModelPath
            it.outputPath = defaultOutputPath
            try {
                it.setGenerationLanguage(defaultLanguage)
            } catch (e: IllegalArgumentException) {
                logger.error("The specified generation language is not valid: $defaultLanguage")
                e.printStackTrace()
            }
            it.generationId = generationId.orNull
            if (addVersionTo.isPresent) {
                it.setAddVersionTo(addVersionTo.get())
            }
            if (target.isPresent) {
                try {
                    it.setTarget(target.get())
                } catch (e: IllegalArgumentException) {
                    logger.error("The specified target is not valid: " + target.get())
                    e.printStackTrace()
                }
            }
            if (extraParameters.isPresent) {
                extraParameters.get().forEach { (key, value) ->
                    run {
                        it.parameter[key] = value
                    }
                }
            }
        }
        return invocationArguments
    }

    fun setClean(doClean: Boolean) {
        this.doClean = doClean
    }

    /**
     * Checks whether all required arguments are present and that the skip argument is not set to true.
     * @return True in case the generator shall be executed. Otherwise false.
     */
    fun shouldGeneratorBeExecuted(): Boolean {
        if (isSkipFlagSet) {
            logger.info("Skipping execution of joynr Generator as skip flag is set!")
            return false
        }
        if (!getGeneratorInvocationArguments().areInvocationArgumentsValid(logger)) return false
        return true
    }
}
