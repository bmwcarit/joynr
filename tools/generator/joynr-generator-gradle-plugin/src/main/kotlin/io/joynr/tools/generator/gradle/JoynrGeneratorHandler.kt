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

import io.joynr.generator.Executor
import org.gradle.api.logging.Logger

open class JoynrGeneratorHandler(
    private val logger: Logger,
    private val joynrGeneratorArgumentHandler: JoynrGeneratorArgumentHandler
) {

    /**
     * Invoke the Joynr Generator with the arguments configured in the corresponding build.gradle file.
     */
    fun execute() {
        val generatorArguments = joynrGeneratorArgumentHandler.getGeneratorInvocationArguments()
        if (!joynrGeneratorArgumentHandler.shouldGeneratorBeExecuted())
            return
        logger.quiet("---> Executing joynr generator plugin...")
        logger.quiet("---> joynr generator FIDL model path: ${generatorArguments.modelPath}")
        val executor = Executor(generatorArguments)
        executor.generate()
        logger.info("joynr generator gradle plugin executed successfully!")
    }
}
