package io.joynr.generator
/*
 * !!!
 *
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import com.google.common.collect.Sets
import com.google.inject.AbstractModule
import com.google.inject.Inject
import com.google.inject.assistedinject.FactoryModuleBuilder
import io.joynr.generator.communicationmodel.CommunicationModelGenerator
import io.joynr.generator.filter.FilterGenerator
import io.joynr.generator.interfaces.InterfaceGenerator
import io.joynr.generator.provider.ProviderGenerator
import io.joynr.generator.proxy.ProxyGenerator
import io.joynr.generator.util.IgnoreSVNFileFilter
import io.joynr.generator.util.JavaTemplateFactory
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import java.io.File
import java.io.FileNotFoundException
import java.util.HashSet
import java.util.Map
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel

import static com.google.common.base.Preconditions.*
import static org.eclipse.xtext.util.Files.*

class JoynrJavaGenerator extends AbstractJoynrGenerator {
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

	@Inject private FrancaPersistenceManager francaPersistenceManager

	override getLanguageId() {
		"java"
	}

	override getGeneratorModule() {
		new AbstractModule() {
			override protected configure() {
				install(new FactoryModuleBuilder().build(JavaTemplateFactory))
			}
		}
	}

	/*
	 * Triggers the generation. In case the parameter "generate" is set to false, the generator is cleaning the generation folder
	 */
	override doGenerate(Resource input, IFileSystemAccess fsa) {
		val isFrancaIDLResource = input.URI.fileExtension.equals(francaPersistenceManager.fileExtension)
		checkArgument(isFrancaIDLResource, "Unknown input: " + input)

		val fModel = input.contents.get(0) as FModel

		for(fInterface: fModel.interfaces){
			interfacesGenerator.doGenerate(fInterface, fsa)
			proxyGenerator.doGenerate(fInterface, fsa)
			providerGenerator.doGenerate(fInterface, fsa)
			filterGenerator.doGenerate(fInterface, fsa)
		}
		//cleanDirectory(containerpath)
		communicationModelGenerator.doGenerate(fModel, fsa)
	}

	def void cleanDirectory(String path) {
		val directory = new File(path);
		if (!directory.exists()) {
			directory.mkdirs();
		} else {
			try {
				cleanFolder(directory, new IgnoreSVNFileFilter(), true, false);
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			}
		}
	}

	def Iterable<FInterface> findAllFInterfaces(Resource resource) {
		val result = new HashSet<FInterface>()
		val rs = resource.resourceSet
		for (r : rs.resources){
			for (c : r.contents){
				if (c instanceof FModel){
					result.addAll((c).interfaces)
				}
			}
		}
		return result
	}

	def Iterable<FCompoundType> findAllComplexTypes(Resource resource) {
		val result = new HashSet<FCompoundType>()
		val rs = resource.resourceSet
		for (r : rs.resources){
			for (c : r.contents){
				if (c instanceof FModel){
					result.addAll(getCompoundDataTypes(c))
				}
			}
		}
		return result
	}

	override setParameters(Map<String,String> parameter) {
		// do nothing
	}

	override supportedParameters() {
		Sets::newHashSet();
	}
}