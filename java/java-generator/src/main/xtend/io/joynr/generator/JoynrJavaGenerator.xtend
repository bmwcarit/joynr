package io.joynr.generator
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import com.google.inject.Inject
import io.joynr.generator.communicationmodel.CommunicationModelGenerator
import io.joynr.generator.interfaces.InterfaceGenerator
import io.joynr.generator.provider.ProviderGenerator
import io.joynr.generator.proxy.ProxyGenerator
import io.joynr.generator.util.IgnoreSVNFileFilter
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import java.io.File
import java.io.FileNotFoundException
import java.util.HashSet
import org.eclipse.core.runtime.Path
import org.eclipse.emf.ecore.plugin.EcorePlugin
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel
import org.franca.core.franca.FType

import static com.google.common.base.Preconditions.*
import static org.eclipse.xtext.util.Files.*
import java.util.Map
import com.google.common.collect.Sets

class JoynrJavaGenerator implements IJoynrGenerator {
	@Inject
	InterfaceGenerator interfacesGenerator
	@Inject
	CommunicationModelGenerator communicationModelGenerator
	@Inject
	ProxyGenerator proxyGenerator
	@Inject
	ProviderGenerator providerGenerator
	
	@Inject extension JoynrJavaGeneratorExtensions
	
	@Inject private FrancaPersistenceManager francaPersistenceManager
	

	override getLanguageId() {
		"java"
	}
	
    override doGenerate(Resource input, IFileSystemAccess fsa) {
        val isFrancaIDLResource = input.URI.fileExtension.equals(francaPersistenceManager.fileExtension)
        checkArgument(isFrancaIDLResource, "Unknown input: " + input)	
	
//        francaGenerator.doGenerate(input, fsa);

        val fModel = input.contents.get(0) as FModel //francaPersistenceManager.loadModel(input.URI, input.URI)
//        val fModel = francaPersistenceManager.loadModel(input.filePath)
        
//		if (fsa instanceof AbstractFileSystemAccess){
//			cleanDirectory((fsa as AbstractFileSystemAccess).outputConfigurations.get(IFileSystemAccess::DEFAULT_OUTPUT).outputDirectory + File::separator + containerpath)
//		}
		for(fInterface: fModel.interfaces){
			interfacesGenerator.doGenerate(fInterface, fsa)
			proxyGenerator.doGenerate(fInterface, fsa)
			providerGenerator.doGenerate(fInterface, fsa)
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
					result.addAll((c as FModel).interfaces)
				}
			}
		}
		return result
	}
	
	    
	def Iterable<FType> findAllComplexTypes(Resource resource) {
		val result = new HashSet<FType>()
		val rs = resource.resourceSet
		for (r : rs.resources){
			for (c : r.contents){
				if (c instanceof FModel){
					result.addAll(getComplexDataTypes(c as FModel))
				}
			}
		}
		return result
	}

    def getFilePath(Resource resource) {
        val root = EcorePlugin::workspaceRoot
        if (resource.URI.file)
            return resource.URI.toFileString

        val platformPath = new Path(resource.URI.toPlatformString(true))
        val file = root.getFile(platformPath)

        return file.location.toString
    }
    
	override setParameters(Map<String,String> parameter) {
		// do nothing
	}
	
	override supportedParameters() {
		Sets::newHashSet();
	}
	
}