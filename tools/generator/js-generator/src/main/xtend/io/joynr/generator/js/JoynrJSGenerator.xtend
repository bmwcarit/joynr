package io.joynr.generator.js

/*
 * !!!
 * 
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
import io.joynr.generator.IJoynrGenerator
import io.joynr.generator.js.communicationmodel.ErrorEnumTypesGenerator
import io.joynr.generator.js.communicationmodel.TypesGenerator
import io.joynr.generator.js.util.GeneratorParameter
import io.joynr.generator.js.util.JsTemplateFactory
import java.util.HashSet
import java.util.Map
import javax.inject.Inject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel
import org.franca.core.franca.FType

import static com.google.common.base.Preconditions.*
import io.joynr.generator.templates.util.SupportedFrancaFeatureChecker

class JoynrJSGenerator implements IJoynrGenerator {

	// @Inject private FrancaPersistenceManager francaPersistenceManager
	@Inject private GeneratorParameter parameters
	@Inject JsTemplateFactory templateFactory
	@Inject private extension TypesGenerator
	@Inject private extension ErrorEnumTypesGenerator

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

	override doGenerate(Resource input, IFileSystemAccess fsa) {
		val isFrancaIDLResource = input.URI.fileExtension.equals(FrancaPersistenceManager.FRANCA_FILE_EXTENSION)
		checkArgument(isFrancaIDLResource, "Unknown input: " + input)

		val fModel = input.contents.get(0) as FModel // francaPersistenceManager.loadModel(input.URI, input.URI)
		val types = findAllFTypes(input)

		SupportedFrancaFeatureChecker.checkModel(fModel)

		for (francaIntf : fModel.interfaces) {
			var proxyGenerator = templateFactory.createProxyGenerator(francaIntf)
			proxyGenerator.generateProxy(fsa)
			var providerGenerator = templateFactory.createProviderGenerator(francaIntf)
			providerGenerator.generateProvider(fsa)
		}
		fModel.interfaces.forEach [
			generateErrorEnumTypes(types, fsa)
		]

		if (!fModel.typeCollections.empty) {
			fModel.typeCollections.forEach [
				generateTypes(it.types, fsa)
			]
		}

//		communicationModelGenerator.doGenerate(directory + File::separator + "communication-model")
//		providerGenerator.doGenerate(directory + File::separator + "provider")
	/*
	 * 		val fModel = francaPersistenceManager.loadModel(input.filePath)
	 * 		fModel.interfaces.forEach[
	 * 			generateDBusProxy(fileSystemAccess)
	 * 			generateDBusStubAdapter(fileSystemAccess)
	 * 		]
	 */
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
