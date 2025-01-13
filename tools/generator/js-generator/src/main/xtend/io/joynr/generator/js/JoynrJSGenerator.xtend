package io.joynr.generator.js

/*
 * !!!
 * 
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
 * 
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
 */

import com.google.inject.AbstractModule
import com.google.inject.assistedinject.FactoryModuleBuilder
import com.google.inject.name.Named
import io.joynr.generator.IJoynrGenerator
import io.joynr.generator.js.communicationmodel.ErrorEnumTypesGenerator
import io.joynr.generator.js.communicationmodel.TypesGenerator
import io.joynr.generator.js.util.GeneratorParameter
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.js.util.JsTemplateFactory
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.SupportedFrancaFeatureChecker
import java.util.HashSet
import java.util.Map
import com.google.inject.Inject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel
import org.franca.core.franca.FType

import static com.google.common.base.Preconditions.*

class JoynrJSGenerator implements IJoynrGenerator {

	// @Inject private FrancaPersistenceManager francaPersistenceManager
	@Inject GeneratorParameter parameters
	@Inject JsTemplateFactory templateFactory
	@Inject extension TypesGenerator
	@Inject extension ErrorEnumTypesGenerator
	@Inject extension JoynrJSGeneratorExtensions

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
		"javascript"
	}

	override getGeneratorModule() {
		new AbstractModule() {
			override protected configure() {
				install(new FactoryModuleBuilder().build(JsTemplateFactory))
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

	override doGenerate(Resource input, IFileSystemAccess fsa) {
		val isFrancaIDLResource = input.URI.fileExtension.equals(FrancaPersistenceManager.FRANCA_FILE_EXTENSION)
		checkArgument(isFrancaIDLResource, "Unknown input: " + input)

		val fModel = input.contents.get(0) as FModel // francaPersistenceManager.loadModel(input.URI, input.URI)
		checkForNamedArrays(fModel, input.URI.path);

		SupportedFrancaFeatureChecker.checkModel(fModel)

		for (francaIntf : fModel.interfaces) {
			checkVersioningOption(francaIntf, packageWithVersion)
			val generateVersioning = if (versioningComment) !commentContainsNoVersionGeneration(francaIntf) else packageWithVersion

			// since the proxy code at the moment contains exported interfaces
			// required to implement a provider, we have to create the proxy code
			// also for the provider.
			if (generateProxyCode || generateProviderCode) {
				var proxyGenerator = templateFactory.createProxyGenerator(francaIntf)
				proxyGenerator.generateProxy(fsa, generateVersioning)
			}

			if (generateProviderCode) {
				var providerGenerator = templateFactory.createProviderGenerator(francaIntf)
				providerGenerator.generateProvider(fsa, generateVersioning)
				var providerImplCreatorGenerator = templateFactory.createProviderImplCreatorGenerator(francaIntf)
				providerImplCreatorGenerator.generateProvider(fsa, generateVersioning)
			}
		}
	}

	override generateCommunicationModel(Resource input, IFileSystemAccess fsa) {
		val fModel = input.contents.get(0) as FModel
		val types = findAllFTypes(input)
		if (generateVersionedCommunicationModel) {
			fModel.interfaces.forEach [
				generateTypes(types, fsa, true)
				generateErrorEnumTypes(types, fsa, true)
			]
		}
		if (generateUnversionedCommunicationModel) {
			fModel.interfaces.forEach [
				generateTypes(types, fsa, false)
				generateErrorEnumTypes(types, fsa, false)
			]
		}
		if (!fModel.typeCollections.empty) {
			if (generateVersionedCommunicationModel) {
				fModel.typeCollections.forEach [
					generateTypes(it.types, fsa, true) // isn't this already included in types = findAllFTypes(input)?
				]
			}
			if (generateUnversionedCommunicationModel) {
				fModel.typeCollections.forEach [
					generateTypes(it.types, fsa, false) // isn't this already included in types = findAllFTypes(input)?
				]
			}
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
					result.addAll(c.interfaces)
				}
			}
		}
		return result
	}

	def Iterable<FType> findAllFTypes(Resource resource) {
		val result = new HashSet<FType>()
		val rs = resource.resourceSet
		for (r : rs.resources) {
			for (c : r.contents) {
				if (c instanceof FModel) {
					for (fi : c.interfaces) {
						result.addAll(fi.types)
					}
					for (tc : c.typeCollections) {
						result.addAll(tc.types)
					}
				}
			}
		}
		return result
	}

	override setParameters(Map<String, String> parameter) {
		parameters.setParameters(parameter);
	}

	override supportedParameters() {
		parameters.supportedParameters();
	}
}
