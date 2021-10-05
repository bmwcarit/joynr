package io.joynr.generator.js.util

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

import com.google.inject.Inject
import io.joynr.generator.templates.util.AbstractTypeUtil
import io.joynr.generator.templates.util.InterfaceUtil.TypeSelector
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import java.nio.file.Path
import java.nio.file.Paths
import java.util.Collections
import java.util.HashMap
import java.util.Map
import java.util.logging.Logger
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FArgument
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.impl.FTypeDefImpl

class JSTypeUtil extends AbstractTypeUtil {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension MethodUtil

	Logger logger = Logger::getLogger("JSTypeUtil")

	Map<FBasicTypeId,String> primitiveDataTypeDefaultMap;

	new () {
	val Map<FBasicTypeId,String> primitiveDataTypeDefaultValue = new HashMap<FBasicTypeId, String>();
		primitiveDataTypeDefaultValue.put(FBasicTypeId::BOOLEAN, "false");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT8, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT8, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT16, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT16, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT32, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT32, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT64, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT64, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::FLOAT, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::DOUBLE, "-1");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::STRING, "\"\"");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UNDEFINED,"");

		primitiveDataTypeDefaultMap = Collections::unmodifiableMap(primitiveDataTypeDefaultValue);
	}

	override isCompound(FType type) {
		if (type instanceof FTypeDef){
			return isCompound(type.actualType)
		}
		return super.isCompound(type)
	}

	override getCompoundType(FType type) {
		if (type instanceof FTypeDef){
			return getCompoundType(type.actualType)
		}
		return super.getCompoundType(type)
	}

	override isPrimitive(FType type) {
		if (type instanceof FTypeDef){
			return isPrimitive(type.actualType)
		}
		return super.isPrimitive(type)
	}

	override getPrimitive(FType type) {
		if (type instanceof FTypeDef){
			return getPrimitive(type.actualType)
		}
		return super.getPrimitive(type)
	}

	override getTypeName(FBasicTypeId datatype) {
		switch (datatype){
			case FBasicTypeId::STRING: return "String"
			case FBasicTypeId::INT8: return "Number"
			case FBasicTypeId::UINT8: return "Number"
			case FBasicTypeId::INT16: return "Number"
			case FBasicTypeId::UINT16: return "Number"
			case FBasicTypeId::INT32: return "Number"
			case FBasicTypeId::UINT32: return "Number"
			case FBasicTypeId::INT64: return "Number"
			case FBasicTypeId::UINT64: return "Number"
			case FBasicTypeId::BOOLEAN: return "Boolean"
			case FBasicTypeId::FLOAT: return "Number"
			case FBasicTypeId::DOUBLE: return "Number"
			case FBasicTypeId::BYTE_BUFFER:return "Array.<Number>"
			default: throw new UnsupportedOperationException("Unsupported basic type: " + datatype.joynrName)
			}
	}
	
	def getTsTypeName(FBasicTypeId datatype) {
		switch (datatype){
			case FBasicTypeId::STRING: return "string"
			case FBasicTypeId::INT8: return "number"
			case FBasicTypeId::UINT8: return "number"
			case FBasicTypeId::INT16: return "number"
			case FBasicTypeId::UINT16: return "number"
			case FBasicTypeId::INT32: return "number"
			case FBasicTypeId::UINT32: return "number"
			case FBasicTypeId::INT64: return "number"
			case FBasicTypeId::UINT64: return "number"
			case FBasicTypeId::BOOLEAN: return "boolean"
			case FBasicTypeId::FLOAT: return "number"
			case FBasicTypeId::DOUBLE: return "number"
			case FBasicTypeId::BYTE_BUFFER:return "number[]"
			default: throw new UnsupportedOperationException("Unsupported basic type: " + datatype.joynrName)
			}
	}

	override getTypeName(FType datatype) {
		if (isEnum(datatype)){
			return datatype.enumType.joynrName;
		}
		if (isPrimitive(datatype)){
			return datatype.getPrimitive.typeName
		}
		if (isCompound(datatype)){
			return datatype.compoundType.joynrName
		}
		if (isMap(datatype)){
			return datatype.mapType.joynrName
		}
		throw new IllegalStateException("JSTypeUtil.getTypeName: unsupported state, datatype " +
			datatype.joynrName + " could not be mapped to an implementation datatype")
	}

	override getTypeName(FType datatype, boolean generateVersion) {
		throw new IllegalStateException("Unsupported method called for JS!")
	}

	override getTypeNameForList(FType datatype, boolean generateVersion) {
		throw new IllegalStateException("Unsupported method called for JS!")
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		"Array." + datatype.typeName
	}

	override getTypeNameForList(FType datatype) {
		"Array." + datatype.typeName
	}

	def checkPropertyTypeName(FTypedElement element) {
		checkPropertyTypeName(element.type, isArray(element))
	}

	def String checkPropertyTypeName(FTypeRef type, boolean isArray) {
		if (isArray || type.byteBuffer) {
			return "\"Array\""
		}
		if (type.isTypeDef){
			return checkPropertyTypeName(type.derived.typeDefType.actualType, type.derived.typeDefType.actualType instanceof FArrayType);
		}
		if (type.isPrimitive) {
			if (type.getPrimitive.bool) {
				return "\"Boolean\""
			}
			if (type.getPrimitive.string) {
				return "\"String\""
			}
			return "\"Number\""
		}
		return  "\"" + type.derived.joynrName + "\""
	}

	def String getJsdocTypeName (FTypedElement typedElement) {
		var result =
				if (isArray(typedElement))
					typedElement.type.jsdocTypeNameForList
				else
					typedElement.type.jsdocTypeName
		if (result === null) {
			throw new IllegalStateException ("Datatype for element " + typedElement.name + " could not be found");
		}
		return result;
	}
	
	def String getTsTypeName (FTypedElement typedElement) {
		var result =
			if (isArray(typedElement))
				typedElement.type.tsTypeNameForList
			else
				typedElement.type.tsTypeName
		if (result === null) {
			throw new IllegalStateException ("Datatype for element " + typedElement.name + " could not be found");
		}
		return result;
	}

	def String getRelativeImportPath (FTypedElement typedElement, FType parentElement, boolean generateVersion) {
		var type = typedElement.type;
		return getRelativeImportPath(type, parentElement, generateVersion);
	}

	def String getRelativeImportPath (FTypeRef type, FType parentElement, boolean generateVersion) {
		if (type.derived !== null){
			var Path childPath;
			var parentPath = Paths.get(parentElement.getDependencyPath(generateVersion));
			if (type.derived instanceof FTypeDefImpl){
				var typeRef = type.derived as FTypeDefImpl;
				if (typeRef.actualType.derived !== null){
					childPath = Paths.get(typeRef.actualType.derived.getDependencyPath(generateVersion));
				} else {
					return null;
				}
			} else {
				childPath = Paths.get(type.derived.getDependencyPath(generateVersion));
			}
			return "./" + parentPath.getParent().relativize(childPath).toString();
		}
		return null;
	}

	def String getRelativeImportPath (FType type, FType parentElement, boolean generateVersion) {
		var parentPath = Paths.get(parentElement.getDependencyPath(generateVersion));
		var childPath = Paths.get(type.getDependencyPath(generateVersion));
		return "./" + parentPath.getParent().relativize(childPath).toString();
	}

	def getJsdocTypeName(FType datatype) {
		if (datatype.isEnum){
			return  datatype.name
		}
		if (datatype.isPrimitive){
			return datatype.getPrimitive.typeName
		}
		if (datatype.isCompound){
			return datatype.compoundType.joynrName
		}
		if (datatype.isMap){
			return datatype.mapType.joynrName
		}
		if (datatype.isTypeDef){
			return datatype.typeDefType.actualType.jsdocTypeName
		}
		throw new IllegalStateException("getJsdocTypeName: unsupported state, datatype " +
			datatype.joynrName + " could not be mapped to an implementation datatype")
	}
	
	def getTsTypeName(FType datatype) {
		// isTypeDef check needs to be first when having a typeDef for an enum, because isEnum will also return true
		if (datatype.isTypeDef){
			return datatype.typeDefType.actualType.tsTypeName
		}
		if (datatype.isEnum){
			return  datatype.name
		}
		if (datatype.isPrimitive){
			return datatype.getPrimitive.tsTypeName
		}
		if (datatype.isCompound){
			return datatype.compoundType.joynrName
		}
		if (datatype.isMap){
			return datatype.mapType.joynrName
		}
		throw new IllegalStateException("getJsdocTypeName: unsupported state, datatype " +
			datatype.joynrName + " could not be mapped to an implementation datatype")
	}

	def String getJsdocTypeName(FTypeRef type) {
		if (type.derived !== null) {
			type.derived.jsdocTypeName
		} else {
			type.predefined.typeName
		}
	}

	def String getTsTypeName(FTypeRef type) {
		if (type.derived !== null) {
			type.derived.tsTypeName
		} else {
			type.predefined.tsTypeName
		}
	}

	def String getTsTypeNameForList(FTypeRef type) {
		if (type.derived !== null) {
			type.derived.tsTypeNameForList
		} else {
			type.predefined.tsTypeNameForList
		}
	}

	def String getJsdocTypeNameForList(FTypeRef type) {
		if (type.derived !== null) {
			type.derived.jsdocTypeNameForList
		} else {
			type.predefined.jsdocTypeNameForList
		}
	}
	def getJsdocTypeNameForList(FBasicTypeId datatype) {
		"Array.<" + datatype.typeName + ">";
	}

	def getTsTypeNameForList(FBasicTypeId datatype) {
		datatype.tsTypeName + "[]";
	}

	def getJsdocTypeNameForList(FType datatype) {
		"Array.<" + datatype.jsdocTypeName + ">";
	}

	def getTsTypeNameForList(FType datatype) {
		datatype.tsTypeName + "[]";
	}

	def appendJSDocSummaryAndWriteSeeAndDescription(FModelElement element, String prefix) '''
		«IF element.comment !== null»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::DESCRIPTION»
					«prefix»<br/><br/>«comment.comment.replaceAll("\n\\s*", "\n" + prefix)»
				«ENDIF»
			«ENDFOR»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::SEE»
					«prefix»@see «comment.comment.replaceAll("\n\\s*", "\n" + prefix)»
				«ENDIF»
				«IF comment.type == FAnnotationType::DETAILS»
					«prefix»@desc «comment.comment.replaceAll("\n\\s*", "\n" + prefix)»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
	'''

	def writeJSDocForSignature(String interfaceName, FMethod operation, String prefix)'''
		«IF operation.inputParameters.size > 0»
			«prefix»@param {Object} settings the arguments object for this function call
			«FOR param: operation.inputParameters»
				«prefix»@param {«param.jsdocTypeName»} settings.«param.joynrName» -
				«IF param.comment!==null»
					«prefix»«FOR comment: param.comment.elements SEPARATOR "<br/>"»«comment.comment.replaceAll("\n\\s*", "\n" + prefix)»«ENDFOR»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
		«IF operation.outputParameters.size>0»
			«prefix»@returns {«interfaceName»#«operation.joynrName.toFirstUpper»Returned}
			«FOR param : operation.outputParameters SEPARATOR "<br/>"»
				«prefix»	{«param.jsdocTypeName»} «param.joynrName»
			«ENDFOR»
		«ENDIF»
	'''

	def writeJSDocTypedefForSignature(String interfaceName, FMethod operation, String operationName, String prefix)'''
		«prefix»@typedef {Object} «interfaceName»#«operationName.toFirstUpper»Returned
		«FOR param : operation.outputParameters»
			«prefix»@property {«param.jsdocTypeName»} «param.joynrName»
			«IF param.comment!==null»
			«prefix»«FOR comment: param.comment.elements SEPARATOR "<br/>"»«comment.comment.replaceAll("\n\\s*", "\n" + prefix)»«ENDFOR»
			«ENDIF»
		«ENDFOR»
	'''

	def getDefaultValue(FTypedElement element) {
		var buffer = new StringBuffer()
		if (isArray(element)){
			buffer.append("[")
		} else {
			//default values are not supported (currently) by the Franca IDL
	//		if (member.getDEFAULTVALUE()!=null && !member.getDEFAULTVALUE().isEmpty()){
	//			if (isEnum(member)){
	//				val ENUMDATATYPETYPE enumDatatype = getDatatype(id) as ENUMDATATYPETYPE
	//				for (ENUMELEMENTTYPE element : getEnumElements(enumDatatype)){
	//					if (element.VALUE == member.DEFAULTVALUE){
	//						return enumDatatype.SHORTNAME.toFirstUpper + "::" + element.SYNONYM
	//					}
	//				}
	//				return getPackagePath(enumDatatype, "::") + "::" + enumDatatype.SHORTNAME.toFirstUpper + "::" +  (enumDatatype.ENUMERATIONELEMENTS.ENUMELEMENT.get(0) as ENUMELEMENTTYPE).SYNONYM
	//			}
	////			else if (isLong(member.getDATATYPEREF().getIDREF())){
	////				return member.getDEFAULTVALUE() + "L"
	////			}
	////			else if (isDouble(member.getDATATYPEREF().getIDREF())){
	////				return member.getDEFAULTVALUE() + "d"
	////			}
	//			else{
	//				return member.getDEFAULTVALUE();
	//			}
	//		} else
			if (element.type.isCompound) {
				buffer.append("new ")
				buffer.append(getCompoundType(element.type).joynrName)
				buffer.append("()");
			} else if (element.type.isMap) {
				buffer.append("new ")
				buffer.append(element.type.mapType.joynrName)
				buffer.append("()");
			} else if (isEnum(element.type)){
				val enumType = getEnumType(element.type);
				val enumElements = enumType.enumerators.iterator
				if (!enumElements.hasNext){
					buffer.append("enum type does not contain elements")
				}
				else{
					buffer.append(enumType.joynrName)
					buffer.append(".")
					buffer.append(enumElements.next.joynrName)
				}
			} else if (isPrimitive(element.type)){
				val primitive = getPrimitive(element.type)
				if (primitiveDataTypeDefaultMap.containsKey(primitive)) {
					buffer.append(primitiveDataTypeDefaultMap.get(primitive))
				}
				else {
					buffer.append("");
				}
	 		} else {
	 			logger.warning("Type for element " + element.joynrName + " could not be mapped to a default value");
	 		}
		}
		if (isArray(element)){
			buffer.append("]")
		}
		return buffer.toString;
	}

	def getExemplaryInstantiationOfInputParameter(FMethod method)
	'''{«FOR param: method.inputParameters SEPARATOR ', '»«getExemplaryInstantiationForArgument(param)»«ENDFOR»}'''

	def getExemplaryInstantiationForArgument(FArgument argument)
	'''"«escapeQuotes(argument.joynrName)»": «getDefaultValue(argument)»«IF isArray(argument)»]«ENDIF»'''

	def getTypeNameForErrorEnumType(FMethod method, FEnumerationType errorEnumType, boolean generateVersion) {
		joynrGenerationPrefix + "." + method.getPackageName(generateVersion) + "." + errorEnumType.joynrName
	}

	def getDependencyPath(FType datatype, boolean generateVersion) {
		return datatype.buildPackagePath("/", true, generateVersion)
					+ "/"
					+ datatype.joynrName
	}

	def getTypeSelectorIncludingErrorTypesAndTransitiveTypes() {
		val selector = TypeSelector::defaultTypeSelector
		selector.transitiveTypes(true)
		selector.errorTypes(true)
		return selector
	}
}
