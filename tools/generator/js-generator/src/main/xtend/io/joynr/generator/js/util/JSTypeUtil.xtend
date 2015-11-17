package io.joynr.generator.js.util

/*
 * !!!
 *
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.util.Collections
import java.util.HashMap
import java.util.Map
import java.util.logging.Logger
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement

class JSTypeUtil extends AbstractTypeUtil {

	@Inject private extension JoynrJSGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension MethodUtil

	Logger logger = Logger::getLogger("JSTypeUtil")

	private Map<FBasicTypeId,String> primitiveDataTypeDefaultMap;

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
			case FBasicTypeId::BYTE_BUFFER:
				throw new UnsupportedOperationException("basicType" +
					datatype.joynrName + " could not be mapped to a primitive type name")
			case FBasicTypeId::UNDEFINED:
				throw new UnsupportedOperationException("basicType" +
					datatype.joynrName + " could not be mapped to a primitive type name")
		}
		throw new UnsupportedOperationException("basicType" +
			datatype.joynrName + " could not be mapped to a primitive type name")
	}

	override getTypeName(FType datatype) {
		if (isEnum(datatype)){
			return getEnumType(datatype).joynrName;
		}
		if (isPrimitive(datatype)){
			return getTypeName(getPrimitive(datatype))
		}
		if (isComplex(datatype)){
			return getComplexType(datatype).joynrName
		}
		throw new IllegalStateException("JoynrJSGeneratorExtensions.getMappedDatatype: unsupported state, datatype " +
			datatype.joynrName + " could not be mapped to an implementation datatype")
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		// unused
		"";
	}

	override getTypeNameForList(FType datatype) {
		// unused
		"";
	}


	def String getJsdocTypeName (FTypedElement typedElement) {
		var result =
				if (typedElement.isArray())
					typedElement.type.jsdocTypeNameForList
				else
					typedElement.type.jsdocTypeName
		if (result == null) {
			throw new IllegalStateException ("Datatype for element " + typedElement.name + " could not be found");
		}
		return result;
	}

	def getJsdocTypeName(FType datatype) {
		if (isEnum(datatype)){
			return  datatype.name
		}
		if (isPrimitive(datatype)){
			return getTypeName(getPrimitive(datatype))
		}
		if (isComplex(datatype)){
			return getComplexType(datatype).joynrName
		}
		throw new IllegalStateException("JoynrJSGeneratorExtensions.getMappedDatatype: unsupported state, datatype " +
			datatype.joynrName + " could not be mapped to an implementation datatype")
	}

	def String getJsdocTypeName(FTypeRef type) {
		if (type.derived != null) {
			type.derived.jsdocTypeName
		} else {
			type.predefined.typeName
		}
	}

	def String getJsdocTypeNameForList(FTypeRef type) {
		if (type.derived != null) {
			type.derived.jsdocTypeNameForList
		} else {
			type.predefined.jsdocTypeNameForList
		}
	}
	def getJsdocTypeNameForList(FBasicTypeId datatype) {
		"Array.<" + datatype.typeName + ">";
	}

	def getJsdocTypeNameForList(FType datatype) {
		"Array.<" + datatype.jsdocTypeName + ">";
	}

	def appendJSDocSummaryAndWriteSeeAndDescription(FModelElement element, String prefix) '''
		«IF element.comment != null»
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

	def writeJSDocForSignature(FMethod operation, String prefix)'''
		«FOR param: operation.inputParameters»
			«prefix»@param {«param.jsdocTypeName»} «param.joynrName» -
			«IF param.comment!=null»
			«prefix»«FOR comment: param.comment.elements»«comment.comment.replaceAll("\n\\s*", "\n" + prefix)»«ENDFOR»
			«ENDIF»
		«ENDFOR»
		«IF operation.outputParameters.size==1»
			«val returnParam = operation.outputParameters.iterator.next»
			«prefix»@returns {«returnParam.typeName»} «returnParam.joynrName» -
			«IF returnParam.comment!=null»
			«prefix»«FOR comment: returnParam.comment.elements»«comment.comment.replaceAll("\n\\s*", "\n" + prefix)»«ENDFOR»
			«ENDIF»
		«ELSE»
			«IF operation.outputParameters.size>1»
				«prefix»@returns {the JS code generator does not support methods with multiple return values}
			«ENDIF»
		«ENDIF»
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
			if (isComplex(element.type)) {
				buffer.append("new ")
				buffer.append(getComplexType(element.type).joynrName)
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

	/**
	 * This method is used for assembling the list of parameter types for the attribute and
	 * operations, the types are mapped to ones that are understood by the joynr framework
	 * when sending requests.
	 */
	def toTypesEnum(FType datatype) {
		if (isPrimitive(datatype)) {
			return toTypesEnum(getPrimitive(datatype))
		} else {
			return datatype.buildPackagePath(".", true) + "." + datatype.joynrName;
		}
	}

	def toTypesEnum(FBasicTypeId basicType) {
		switch (basicType){
		case FBasicTypeId::STRING: return "TypesEnum.STRING"
		case FBasicTypeId::INT8: return "TypesEnum.BYTE"
		case FBasicTypeId::UINT8: return "TypesEnum.BYTE"
		case FBasicTypeId::INT16: return "TypesEnum.SHORT"
		case FBasicTypeId::UINT16: return "TypesEnum.SHORT"
		case FBasicTypeId::INT32: return "TypesEnum.INT"
		case FBasicTypeId::UINT32: return "TypesEnum.INT"
		case FBasicTypeId::INT64: return "TypesEnum.LONG"
		case FBasicTypeId::UINT64: return "TypesEnum.LONG"
		case FBasicTypeId::BOOLEAN: return "TypesEnum.BOOL"
		case FBasicTypeId::FLOAT: return "TypesEnum.FLOAT"
		case FBasicTypeId::DOUBLE: return "TypesEnum.DOUBLE"
		case FBasicTypeId::BYTE_BUFFER:
			throw new UnsupportedOperationException("basicType" + basicType.joynrName +
				" could not be mapped to a primitive type name")
		case FBasicTypeId::UNDEFINED:
			throw new UnsupportedOperationException("basicType" + basicType.joynrName +
				" could not be mapped to a primitive type name")
		}
		throw new UnsupportedOperationException("basicType" + basicType.joynrName +
			" could not be mapped to a primitive type name")
	}

	private def getTypeNameForListParameter(String typeName) {
		"\"" + typeName + "[]\""
	}

	def getTypeNameForParameter(FType datatype, boolean array) {
		val mappedDatatype = toTypesEnum(datatype);
		var result = mappedDatatype;

		// special cases: ByteBuffer => byte-array, arrays => Lists,
		if (array || (getPrimitive(datatype) == FBasicTypeId::BYTE_BUFFER)) {
			return getTypeNameForListParameter(result);
		}

		if (!isPrimitive(datatype)) {
			return "\"" + result +  "\"";
		}
	}

	def getTypeNameForParameter(FBasicTypeId datatype, boolean array) {
		val mappedDatatype = toTypesEnum(datatype);
		if (array) {
			return getTypeNameForListParameter(mappedDatatype);
		} else {
			return mappedDatatype;
		}
	}

	def String getTypeNameForParameter(FTypedElement typedElement){
		if (typedElement.type.derived != null){
			getTypeNameForParameter(typedElement.type.derived, typedElement.isArray())
		}
		else{
			getTypeNameForParameter(typedElement.type.predefined, typedElement.isArray())
		}
	}

	def getExemplaryInstantiationOfInputParameter(FMethod method)
	'''{«FOR param: method.inputParameters SEPARATOR ', '»«getExemplaryInstantiationForArgument(param)»«ENDFOR»}'''

	def getExemplaryInstantiationForArgument(FArgument argument)
	'''"«escapeQuotes(argument.joynrName)»": «getDefaultValue(argument)»«IF isArray(argument)»]«ENDIF»'''

	def getTypeNameForErrorEnumType(FMethod method, FEnumerationType errorEnumType) {
		joynrGenerationPrefix + "." + method.packageName + "." + errorEnumType.joynrName
	}
}
