package io.joynr.generator

/*
 * !!!
 * 
 * Copyright (C) 2011 - 2020 BMW Car IT GmbH
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
import com.google.inject.Inject
import com.google.inject.Singleton
import com.google.inject.assistedinject.FactoryModuleBuilder
import com.google.inject.name.Named
import io.joynr.generator.communicationmodel.CommunicationModelGenerator
import io.joynr.generator.filter.FilterGenerator
import io.joynr.generator.interfaces.InterfaceGenerator
import io.joynr.generator.provider.ProviderGenerator
import io.joynr.generator.proxy.ProxyGenerator
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.SupportedFrancaFeatureChecker
import io.joynr.generator.util.JavaTemplateFactory
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import java.util.Arrays
import java.util.HashSet
import java.util.Map
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel

import static com.google.common.base.Preconditions.*

class JoynrJavaGenerator implements IJoynrGenerator {
	@Inject
	InterfaceGenerator interfacesGenerator

	@Inject
	CommunicationModelGenerator communicationModelGenerator

	@Inject
	ProxyGenerator proxyGenerator

	@Inject
	ProviderGenerator providerGenerator

	@Inject
	FilterGenerator filterGenerator

	@Inject extension JoynrJavaGeneratorExtensions

	@Inject
	@Named(NamingUtil.JOYNR_GENERATOR_NOVERSIONGENERATION_COMMENT)
	public boolean versioningComment;

	@Inject
	@Named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION)
	public boolean packageWithVersion;

	@Inject
	@Named("generateProxyCode")
	public boolean generateProxyCode;

	@Inject
	@Named("generateProviderCode")
	public boolean generateProviderCode;

	boolean generateVersionedCommunicationModel = false;
	boolean generateUnversionedCommunicationModel = false;

	override getLanguageId() {
		"java"
	}

	override getGeneratorModule() {
		new AbstractModule() {
			override protected configure() {
				install(new FactoryModuleBuilder().build(JavaTemplateFactory))
				bind(JoynrJavaGeneratorExtensions).in(Singleton)
			}
		}
	}

	override updateCommunicationModelGeneration(Resource input) {
		val fModel = input.contents.get(0) as FModel
		if(fModel.interfaces.size == 0) {
			generateVersionedCommunicationModel = true;
		}
		for (fInterface : fModel.interfaces) {
			checkVersioningOption(fInterface, packageWithVersion)
			val generateVersioning = if (versioningComment) !commentContainsNoVersionGeneration(fInterface) else packageWithVersion
			if (generateVersioning) {
				generateVersionedCommunicationModel = true
			} else {
				generateUnversionedCommunicationModel = true
			}
		}
	}

	/*
	 * Triggers the generation. In case the parameter "generate" is set to false, the generator is cleaning the generation folder
	 */
	override doGenerate(Resource input, IFileSystemAccess fsa) {
		val isFrancaIDLResource = input.URI.fileExtension.equals(FrancaPersistenceManager.FRANCA_FILE_EXTENSION)
		checkArgument(isFrancaIDLResource, "Unknown input: " + input)

		val fModel = input.contents.get(0) as FModel
		checkForNamedArrays(fModel, input.URI.path);

		SupportedFrancaFeatureChecker.checkModel(fModel)

		for (fInterface : fModel.interfaces) {
			val generateVersioning = if (versioningComment) !commentContainsNoVersionGeneration(fInterface) else packageWithVersion
			checkVersioningOption(fInterface, packageWithVersion)
			interfacesGenerator.doGenerate(fInterface, fsa, generateVersioning)
			if (generateProxyCode) {
				proxyGenerator.doGenerate(fInterface, fsa, generateVersioning)
			}
			if (generateProviderCode) {
				providerGenerator.doGenerate(fInterface, fsa, generateVersioning)
				filterGenerator.doGenerate(fInterface, fsa, generateVersioning)
			}
		}
		// cleanDirectory(containerpath)
	}

	override generateCommunicationModel(Resource input, IFileSystemAccess fsa) {
		val fModel = input.contents.get(0) as FModel
		if (generateVersionedCommunicationModel) {
			communicationModelGenerator.doGenerate(fModel, fsa, true)
		}
		if (generateUnversionedCommunicationModel) {
			communicationModelGenerator.doGenerate(fModel, fsa, false)
		}
	}

	override clearCommunicationModelGenerationSettings() {
		generateVersionedCommunicationModel = false;
		generateUnversionedCommunicationModel = false;
	}

	def Iterable<FInterface> findAllFInterfaces(Resource resource) {
		val result = new HashSet<FInterface>()
		val rs = resource.resourceSet
		for (r : rs.resources) {
			for (c : r.contents) {
				if (c instanceof FModel) {
					result.addAll((c).interfaces)
				}
			}
		}
		return result
	}

	def Iterable<FCompoundType> findAllComplexTypes(Resource resource) {
		val result = new HashSet<FCompoundType>()
		val rs = resource.resourceSet
		for (r : rs.resources) {
			for (c : r.contents) {
				if (c instanceof FModel) {
					result.addAll(getCompoundDataTypes(c))
				}
			}
		}
		return result
	}

	override setParameters(Map<String, String> parameter) {
		if (parameter.get("ignoreInvalidNullClassMembers") !== null &&
			parameter.get("ignoreInvalidNullClassMembers").equals("true")) {
			activateIgnoreInvalidNullClassMembersExtension
		}
	}

	override supportedParameters() {
		new HashSet(Arrays.asList("jee", "ignoreInvalidNullClassMembers"));
	}
}
