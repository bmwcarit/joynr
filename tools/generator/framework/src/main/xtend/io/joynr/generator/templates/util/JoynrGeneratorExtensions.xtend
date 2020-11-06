package io.joynr.generator.templates.util

/*
 * !!!
 * 
 * Copyright (C) 2020 BMW Car IT GmbH
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
import com.google.inject.name.Named
import io.joynr.generator.templates.BroadcastTemplate
import io.joynr.generator.templates.CompoundTypeTemplate
import io.joynr.generator.templates.EnumTemplate
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.MapTemplate
import io.joynr.generator.templates.TypeDefTemplate
import java.util.Arrays
import java.util.HashSet
import org.eclipse.emf.ecore.impl.BasicEObjectImpl
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModel
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FAnnotationType

class JoynrGeneratorExtensions {

	public final static String JOYNR_GENERATOR_GENERATE = "JOYNR_GENERATOR_GENERATE";
	public final static String JOYNR_GENERATOR_CLEAN = "JOYNR_GENERATOR_CLEAN";

	@Inject
	protected extension NamingUtil;

	@Inject
	protected extension TypeUtil;

	@Inject
	@Named(JOYNR_GENERATOR_GENERATE)
	public boolean generate;

	@Inject
	@Named(JOYNR_GENERATOR_CLEAN)
	public boolean clean;

	def Iterable<FInterface> getInterfaces(FModel model) {
		return model.interfaces
	}

	def String getPackageNameInternal(FModelElement fModelElement, boolean useOwnName, boolean generateVersion) {
		if (fModelElement === null) {
			throw new IllegalStateException(
				"Generator could not proceed with code generation, since JoynGeneratorExtensions.getPackageNameInternal has been invoked with an empty model element");
		} else if (fModelElement.eContainer === null) {
			val errorMsg = "Generator could not proceed with code generation, since " +
				if (fModelElement.joynrName !== null)
					"the container of model element " + fModelElement.joynrName + " is not known"
				else
					"the resource " + (if (fModelElement instanceof BasicEObjectImpl)
						(fModelElement as BasicEObjectImpl).eProxyURI
					else
						fModelElement.eResource.toString) + " cannot be parsed correctly"
			throw new IllegalStateException(errorMsg);
		} else if (fModelElement.eContainer instanceof FModel) {
			return (fModelElement.eContainer as FModel).joynrName + getVersionSuffix(fModelElement, generateVersion);
		} else if (fModelElement instanceof FMethod) {
			// include interface name for unnamed error enums (defined or extended inside method definition)
			val finterface = fModelElement.eContainer as FModelElement
			if (finterface === null || !(finterface instanceof FInterface)) {
				val errorMsg = "Generator could not proceed with code generation, since " +
					"JoynGeneratorExtensions.getPackageNameInternal has been invoked " +
					"with a FMethod element which is not defined inside an interface"
				throw new IllegalStateException(errorMsg);
			}
			return finterface.getPackageNameInternal(false, generateVersion) + '.' + finterface.joynrName
		}
		return (fModelElement.eContainer as FModelElement).getPackageNameInternal(true, generateVersion) +
			(if (useOwnName) '.' + fModelElement.joynrName else '')
	}

	def getPackageName(FModelElement fModelElement, boolean generateVersion) {
		getPackageNameInternal(fModelElement, false, generateVersion)
	}

	def getPackageNames(FModelElement fModelElement, boolean generateVersion) {
		getPackageNames(fModelElement, "\\.", generateVersion)
	}

	def getPackageNames(FModelElement fModelElement, String separator, boolean generateVersion) {
		Arrays::stream(fModelElement.getPackageName(generateVersion).split(separator)).iterator()
	}

	def getPackagePathWithJoynrPrefix(FType datatype, String separator, boolean includeTypeCollection, boolean generateVersion) {
		var packagePath = getPackagePathWithJoynrPrefix(datatype, separator, generateVersion);
		if (includeTypeCollection && datatype.isPartOfNamedTypeCollection) {
			packagePath += separator + datatype.typeCollectionName;
		}
		return packagePath
	}

	def getPackagePathWithJoynrPrefix(FModelElement fModelElement, String separator, boolean generateVersion) {
		joynrGenerationPrefix + separator + getPackagePathWithoutJoynrPrefix(fModelElement, separator, generateVersion)
	}

	def getPackagePathWithoutJoynrPrefix(FModelElement fModelElement, String separator, boolean generateVersion) {
		return getPackageName(fModelElement, generateVersion).replace('.', separator)
	}

	def getFullyQualifiedName(FInterface fInterface) {
		(fInterface.eContainer as FModel).joynrName.replace('.', '/') + '/' + fInterface.name
	}

	def getDataTypes(FModel fModel) {
		val referencedFTypes = new HashSet<FType>()

		fModel.typeCollections.forEach[typeCollection | referencedFTypes.addAll(typeCollection.types)]

		fModel.interfaces.forEach[anInterface | referencedFTypes.addAll(anInterface.types)]

		return referencedFTypes
	}

	def getCompoundDataTypes(FModel fModel) {
		getDataTypes(fModel).filter(type | type.isCompound).map(type | type.compoundType).filterNull
	}

	def getPrimitiveDataTypes() {
		FBasicTypeId::values.filter[value != FBasicTypeId::UNDEFINED_VALUE && value != FBasicTypeId::BYTE_BUFFER_VALUE] // filter out "undefined" and "buffer" data type
	}

	def getEnumDataTypes(FModel fModel) {
		getDataTypes(fModel).filter(type | type.isEnum).map(type | type.enumType).filterNull
	}

	def getTypeDefDataTypes(FModel fModel) {
		getDataTypes(fModel).filter(type | type.isTypeDef).map(type | type.typeDefType).filterNull
	}

	def getMapDataTypes(FModel fModel) {
		getDataTypes(fModel).filter(type | type.isMap).map(type | type.mapType).filterNull
	}

	def prependCommaIfNotEmpty(String input) {
		if (input.equals("")) {
			return input;
		}
		return ", " + input;
	}

	def escapeQuotes(String string) {
		string.replace('\"', '\\\"')
	}

	def getJoynrGenerationPrefix() {
		"joynr"
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		InterfaceTemplate generator,
		boolean generateVersion
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(generateVersion).toString);
		}
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		BroadcastTemplate generator,
		FInterface serviceInterface,
		FBroadcast broadcast,
		boolean generateVersion
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(serviceInterface, broadcast, generateVersion).toString);
		}
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		EnumTemplate generator,
		boolean generateVersion
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(generateVersion).toString);
		}
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		MapTemplate generator,
		boolean generateVersion
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(generateVersion).toString);
		}
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		CompoundTypeTemplate generator,
		boolean generateVersion
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(generateVersion).toString);
		}
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		TypeDefTemplate generator,
		FTypeDef typeDefType,
		boolean generateVersion
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(typeDefType, generateVersion).toString);
		}
	}

	// Convert a data type declaration into a string giving the typename
	def String getJoynrTypeName(FTypedElement element, boolean generateVersion) {
		var typeName = getJoynrTypeName(element.type, generateVersion)
		if (isArray(element)) {
			typeName += "[]"
		}
		return typeName
	}

	def String getJoynrTypeName(FType type, boolean generateVersion) {
		buildPackagePath(type, ".", true, generateVersion) + "." + type.joynrName
	}

	def String getJoynrTypeName(FBasicTypeId predefined) {
		switch predefined {
			case isString(predefined)    : "String"
			case isShort(predefined)     : "Short"
			case isInteger(predefined)   : "Integer"
			case isLong(predefined)      : "Long"
			case isDouble(predefined)    : "Double"
			case isFloat(predefined)     : "Float"
			case isBool(predefined)      : "Boolean"
			case isByte(predefined)      : "Byte"
			case isByteBuffer(predefined): "Byte[]"
			default: throw new RuntimeException("Unhandled primitive type: " + predefined.getName)
		}
	}

	def String getJoynrTypeName(FTypeRef datatypeRef, boolean generateVersion) {
		if (datatypeRef.isTypeDef) {
			getJoynrTypeName((datatypeRef.derived as FTypeDef).actualType, generateVersion)
		} else if (datatypeRef.complex) {
			getJoynrTypeName(datatypeRef.derived, generateVersion)
		} else {
			getJoynrTypeName(datatypeRef.getPrimitive)
		}
	}

	def buildPackagePath(FType datatype, String separator, boolean generateVersion) {
		return buildPackagePath(datatype, separator, false, generateVersion);
	}

	def buildPackagePath(FType datatype, String separator, boolean includeTypeCollection, boolean generateVersion) {
		if (datatype === null) {
			return "";
		}
		var packagepath = "";
		try {
			packagepath = getPackagePathWithJoynrPrefix(datatype, separator, generateVersion);
		} catch (IllegalStateException e) {
			// if an illegal StateException has been thrown, we tried to get the package for a primitive type, so the packagepath stays empty.
		}
		if (includeTypeCollection && datatype.partOfNamedTypeCollection) {
			packagepath += separator + datatype.typeCollectionName;
		}
		return packagepath;
	}

	def checkForNamedArrays(FModel fModel, String path) {
		val allTypes = fModel.dataTypes
		for (fType : allTypes) {
			if (fType instanceof FArrayType) {
				throw new IllegalArgumentException(
					"Explicitly named arrays (\"array NAME of TYPE\") are not supported by the generator: found array \""
					 + fType.name + "\" in " + path)
			}
		}
	}

	def boolean commentContainsNoVersionGeneration(FInterface fInterface){
		if (fInterface.comment === null) {
			return false
		}
		for (comment : fInterface.comment.elements) {
			if (comment.type == FAnnotationType::DESCRIPTION && comment.rawText.contains("#noVersionGeneration")) {
				return true
			}
		}
		return false
	}

	def checkVersioningOption(FInterface fInterface, boolean packageWithVersion) {
		if(commentContainsNoVersionGeneration(fInterface) && (packageWithVersion)) {
			println(
				"ERROR: --addVersionTo option is set " +
				" despite #noVersionGeneration being set for interface "
				+ fInterface.name + ". Please decide whether you want to generate versioning and " +
				" adjust your settings accordingly (Only package versioning is supported)."
			)
			throw new IllegalArgumentException("--addVersionTo is not 'none' despite #noVersionGeneration being set in the fidl.");
		}
	}
}
