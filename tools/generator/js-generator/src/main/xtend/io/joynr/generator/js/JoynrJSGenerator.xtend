package io.joynr.generator.js

/*
 * !!!
 *
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import io.joynr.generator.AbstractJoynrGenerator
import io.joynr.generator.js.communicationmodel.ErrorEnumTypesGenerator
import io.joynr.generator.js.communicationmodel.TypesGenerator
import io.joynr.generator.js.provider.ProviderGenerator
import io.joynr.generator.js.proxy.ProxyGenerator
import io.joynr.generator.js.util.GeneratorParameter
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

class JoynrJSGenerator extends AbstractJoynrGenerator {

	@Inject
	private FrancaPersistenceManager francaPersistenceManager

	@Inject
	private GeneratorParameter parameters

	@Inject
	private extension ProxyGenerator

	@Inject
	private extension ProviderGenerator

	@Inject
	private extension TypesGenerator

	@Inject
	private extension ErrorEnumTypesGenerator

	override getLanguageId() {
		"javascript"
	}

	override doGenerate(Resource input, IFileSystemAccess fsa) {
		val isFrancaIDLResource = input.URI.fileExtension.equals(francaPersistenceManager.fileExtension)
		checkArgument(isFrancaIDLResource, "Unknown input: " + input)

		val fModel = input.contents.get(0) as FModel //francaPersistenceManager.loadModel(input.URI, input.URI)
		val types = findAllFTypes(input)
		fModel.interfaces.forEach[
			generateProxy(types, fsa)
			generateProvider(types, fsa)
			generateErrorEnumTypes(types, fsa)
		]

		if (!fModel.typeCollections.empty) {
			fModel.typeCollections.forEach[
				generateTypes(it.types, fsa)
			]
		}


//		communicationModelGenerator.doGenerate(directory + File::separator + "communication-model")
//		providerGenerator.doGenerate(directory + File::separator + "provider")
/*
		val fModel = francaPersistenceManager.loadModel(input.filePath)
		fModel.interfaces.forEach[
			generateDBusProxy(fileSystemAccess)
			generateDBusStubAdapter(fileSystemAccess)
		]
*/
	}

	def Iterable<FInterface> findAllFInterfaces(Resource resource) {
		val result = new HashSet<FInterface>()
		val rs = resource.resourceSet
		for (r : rs.resources){
			for (c : r.contents){
				if (c instanceof FModel){
					result.addAll(c.interfaces)
				}
			}
		}
		return result
	}

	def Iterable<FType> findAllFTypes(Resource resource) {
		val result = new HashSet<FType>()
		val rs = resource.resourceSet
		for (r : rs.resources){
			for (c : r.contents){
				if (c instanceof FModel){
					for(fi : c.interfaces){
						result.addAll(fi.types)
					}
					for(tc : c.typeCollections){
						result.addAll(tc.types)
					}
				}
			}
		}
		return result
	}

	override setParameters(Map<String,String> parameter) {
		parameters.setParameters(parameter);
	}

	override supportedParameters() {
		parameters.supportedParameters();
	}
}
