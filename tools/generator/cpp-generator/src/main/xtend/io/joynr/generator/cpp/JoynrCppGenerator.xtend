package io.joynr.generator.cpp

/*
 * !!!
 * 
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import com.google.inject.AbstractModule
import com.google.inject.assistedinject.FactoryModuleBuilder
import com.google.inject.name.Named
import io.joynr.generator.IJoynrGenerator
import io.joynr.generator.cpp.communicationmodel.CommunicationModelGenerator
import io.joynr.generator.cpp.defaultProvider.DefaultInterfaceProviderGenerator
import io.joynr.generator.cpp.filter.FilterGenerator
import io.joynr.generator.cpp.joynrmessaging.JoynrMessagingGenerator
import io.joynr.generator.cpp.provider.ProviderGenerator
import io.joynr.generator.cpp.proxy.ProxyGenerator
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.TypeUtil
import io.joynr.generator.templates.util.JoynrGeneratorExtensions
import io.joynr.generator.util.FileSystemAccessUtil
import io.joynr.generator.util.InvocationArguments
import java.io.File
import java.util.Map
import java.util.Arrays
import java.util.HashSet
import javax.inject.Inject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FModel

import static com.google.common.base.Preconditions.*
import io.joynr.generator.cpp.util.CppTemplateFactory
import io.joynr.generator.templates.util.SupportedFrancaFeatureChecker

class JoynrCppGenerator implements IJoynrGenerator{
	@Inject CommunicationModelGenerator communicationModelGenerator
	@Inject ProxyGenerator proxyGenerator
	@Inject ProviderGenerator providerGenerator
	@Inject FilterGenerator filterGenerator;
	@Inject JoynrMessagingGenerator joynrMessagingGenerator
	@Inject DefaultInterfaceProviderGenerator defaultProviderGenerator

	@Inject IFileSystemAccess outputHeaderFileSystem;

	@Inject
	protected extension JoynrGeneratorExtensions;

	@Inject
	@Named(NamingUtil.JOYNR_GENERATOR_NAMEWITHVERSION)
	public boolean nameWithVersion;

	@Inject
	@Named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION)
	public boolean packageWithVersion;

	public static final String OUTPUT_HEADER_PATH = "outputHeaderPath";
	Map<String, String> parameters;

	override doGenerate(Resource input, IFileSystemAccess fsa) {
		doGenerate(input, fsa, getHeaderFileSystemAccess(fsa));
	}

	override getLanguageId() {
		return "cpp"
	}

	override getGeneratorModule() {
		new AbstractModule() {
			override protected configure() {
				bind(typeof(TypeUtil)).to(typeof(CppStdTypeUtil))
				install(new FactoryModuleBuilder().build(CppTemplateFactory))
			}
		}
	}

	/*
	 * Triggers the generation. In case the parameter "generate" is set to false, the generator is cleaning the generation folder
	 */
	def doGenerate(Resource input, IFileSystemAccess sourceFileSystem, IFileSystemAccess headerFileSystem) {
		val fModel = getModel(input);

		for (fInterface : fModel.interfaces) {
			printVersionWarnings(fInterface, packageWithVersion, nameWithVersion)
		}

		SupportedFrancaFeatureChecker.checkModel(fModel)

		filterGenerator.doGenerate(
			fModel,
			sourceFileSystem,
			headerFileSystem,
			getSourceContainerPath(sourceFileSystem, "filter"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "filter")
		);

		proxyGenerator.doGenerate(
			fModel,
			sourceFileSystem,
			headerFileSystem,
			getSourceContainerPath(sourceFileSystem, "proxy"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "proxy")
		);

		providerGenerator.doGenerate(
			fModel,
			sourceFileSystem,
			headerFileSystem,
			getSourceContainerPath(sourceFileSystem, "provider"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "provider")
		);

		defaultProviderGenerator.doGenerate(
			fModel,
			sourceFileSystem,
			headerFileSystem,
			getSourceContainerPath(sourceFileSystem, "provider"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "provider")
		);

		joynrMessagingGenerator.doGenerate(
			fModel,
			sourceFileSystem,
			headerFileSystem,
			getSourceContainerPath(sourceFileSystem, "joynr-messaging"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "joynr-messaging")
		);

		communicationModelGenerator.doGenerate(
			fModel,
			sourceFileSystem,
			headerFileSystem,
			getSourceContainerPath(sourceFileSystem, "communication-model"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "communication-model")
		);
	}

	def getSourceContainerPath(IFileSystemAccess sourceFileSystem, String directory) {
		return directory + File::separator + "generated" + File::separator
	}

	def getHeaderContainerPath(IFileSystemAccess sourceFileSystem, IFileSystemAccess headerFileSystem,
		String directory) {
		if (sourceFileSystem == headerFileSystem) {
			return getSourceContainerPath(sourceFileSystem, directory);
		} else {
			return "";
		}
	}

	override setParameters(Map<String, String> parameter) {
		parameters = parameter;
	}

	override supportedParameters() {
		new HashSet(Arrays.asList(OUTPUT_HEADER_PATH));
	}

	def getOutputHeaderPath() {
		var String result = null;
		if (parameters !== null) {
			if (parameters.get(OUTPUT_HEADER_PATH) !== null) {
				result = parameters.get(OUTPUT_HEADER_PATH);
			} else if (parameters.get(InvocationArguments::OUTPUT_PATH) !== null) {
				result = parameters.get(InvocationArguments::OUTPUT_PATH) + File::separator + "include"
			}
		}
		return result
	}

	def getHeaderFileSystemAccess(IFileSystemAccess fsa) {
		var headerFSA = fsa;
		if (outputHeaderPath !== null) {
			FileSystemAccessUtil::createFileSystemAccess(outputHeaderFileSystem, outputHeaderPath);
			headerFSA = outputHeaderFileSystem;
		}
		headerFSA
	}

	def getModel(Resource input) {
		val isFrancaIDLResource = input.URI.fileExtension.equals(FrancaPersistenceManager.FRANCA_FILE_EXTENSION)
		checkArgument(isFrancaIDLResource, "Unknown input: " + input)
		return input.contents.get(0) as FModel;
	}
}
