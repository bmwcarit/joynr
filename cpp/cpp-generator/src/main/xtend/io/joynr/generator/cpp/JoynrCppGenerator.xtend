package io.joynr.generator.cpp
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import io.joynr.generator.IJoynrGenerator
import io.joynr.generator.cpp.communicationmodel.CommunicationModelGenerator
import io.joynr.generator.cpp.defaultProvider.DefaultProviderGenerator
import io.joynr.generator.cpp.inprocess.InProcessGenerator
import io.joynr.generator.cpp.joynrmessaging.JoynrMessagingGenerator
import io.joynr.generator.cpp.provider.ProviderGenerator
import io.joynr.generator.cpp.proxy.ProxyGenerator
import java.io.File
import java.util.Map
import javax.inject.Inject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FModel

import static com.google.common.base.Preconditions.*
import io.joynr.generator.util.FileSystemAccessUtil
import io.joynr.generator.util.InvocationArguments

class JoynrCppGenerator implements IJoynrGenerator{

	@Inject 
	private FrancaPersistenceManager francaPersistenceManager

	@Inject 
	CommunicationModelGenerator communicationModelGenerator

	@Inject
	ProxyGenerator proxyGenerator	

	@Inject
	ProviderGenerator providerGenerator

	@Inject
	InProcessGenerator inProcessGenerator

	@Inject
	JoynrMessagingGenerator joynrMessagingGenerator

	@Inject
	DefaultProviderGenerator defaultProviderGenerator

	@Inject
	IFileSystemAccess outputHeaderFileSystem;

	public static final String OUTPUT_HEADER_PATH = "outputHeaderPath";
	private Map<String, String> parameters;

	override doGenerate(Resource input, IFileSystemAccess fsa) {
		var headerFSA = fsa;
		if (outputHeaderPath != null){
			FileSystemAccessUtil::createFileSystemAccess(outputHeaderFileSystem, outputHeaderPath);
			headerFSA = outputHeaderFileSystem;
		}
		
		doGenerate(input, fsa, headerFSA);
	}
	
	override getLanguageId() {
		return "cpp"
	}
	
	def doGenerate(Resource input, IFileSystemAccess sourceFileSystem, IFileSystemAccess headerFileSystem) {
		val isFrancaIDLResource = input.URI.fileExtension.equals(francaPersistenceManager.fileExtension)
		checkArgument(isFrancaIDLResource, "Unknown input: " + input)	

		val fModel = input.contents.get(0) as FModel; 
		
		proxyGenerator.doGenerate(fModel, sourceFileSystem, headerFileSystem, 
			getSourceContainerPath(sourceFileSystem, "proxy"), 
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "proxy")
		);

		providerGenerator.doGenerate(fModel, sourceFileSystem, headerFileSystem, 
			getSourceContainerPath(sourceFileSystem, "provider"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "provider")
		);
		
		defaultProviderGenerator.doGenerate(fModel, sourceFileSystem, headerFileSystem, 
			getSourceContainerPath(sourceFileSystem, "provider"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "provider")
		);
		
		joynrMessagingGenerator.doGenerate(fModel, sourceFileSystem, headerFileSystem, 
			getSourceContainerPath(sourceFileSystem, "joynr-messaging"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "joynr-messaging")
		);
		
		inProcessGenerator.doGenerate(fModel, sourceFileSystem, headerFileSystem, 
			getSourceContainerPath(sourceFileSystem, "in-process"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "in-process")
		);

		communicationModelGenerator.doGenerate(fModel, sourceFileSystem, headerFileSystem, 
			getSourceContainerPath(sourceFileSystem, "communication-model"),
			getHeaderContainerPath(sourceFileSystem, headerFileSystem, "communication-model")
		);
	}

	def getSourceContainerPath(IFileSystemAccess sourceFileSystem, String directory) {
		return directory + File::separator + "generated" + File::separator
	}
	
	def getHeaderContainerPath(IFileSystemAccess sourceFileSystem, IFileSystemAccess headerFileSystem, String directory) {
		if (sourceFileSystem == headerFileSystem) {
			return getSourceContainerPath(sourceFileSystem, directory);
		} else {
			return "";
		}
	}
	
	override setParameters(Map<String,String> parameter) {
		parameters = parameter;
	}
	
	override supportedParameters() {
		Sets::newHashSet(OUTPUT_HEADER_PATH);
	}
	
	def getOutputHeaderPath(){
		var String result = null;
		if (parameters != null) {
			if (parameters.get(OUTPUT_HEADER_PATH) != null) {
				result = parameters.get(OUTPUT_HEADER_PATH);
			}
			else if (parameters.get(InvocationArguments::OUTPUT_PATH) != null){
				result = parameters.get(InvocationArguments::OUTPUT_PATH) + File::separator + "include"
			}
		}
		return result
	}

}
