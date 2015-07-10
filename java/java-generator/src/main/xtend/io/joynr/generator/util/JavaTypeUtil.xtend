package io.joynr.generator.util

import com.google.inject.Inject
import java.util.Collections
import java.util.HashMap
import java.util.Map
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import org.franca.core.franca.FTypedElement

class JavaTypeUtil extends TypeUtil {

	@Inject private extension JoynrJavaGeneratorExtensions

	private Map<FBasicTypeId,String> primitiveDataTypeDefaultMap;

	new () {
	val Map<FBasicTypeId,String> primitiveDataTypeDefaultValue = new HashMap<FBasicTypeId, String>();
		primitiveDataTypeDefaultValue.put(FBasicTypeId::BOOLEAN, "false");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT8, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT8, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT16, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT16, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT32, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT32, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT64, "0L");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT64, "0l");
		//see bug JOYN-1521: floats are interpreted as double
		primitiveDataTypeDefaultValue.put(FBasicTypeId::FLOAT, "0d");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::DOUBLE, "0d");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::STRING, "\"\"");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::BYTE_BUFFER, "new byte[0]");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UNDEFINED,"");

		primitiveDataTypeDefaultMap = Collections::unmodifiableMap(primitiveDataTypeDefaultValue);
	}

	/**
	 * @return a method signature that is unique in terms of method name, out
	 *      parameter names and out parameter types.
	 */
	def createMethodSignature(FMethod method) {
		val nameStringBuilder = new StringBuilder(method.name);
		for (FArgument outParam : method.outputParameters) {
			nameStringBuilder.append(outParam.name.toFirstUpper);
			val typeName = new StringBuilder(outParam.typeName.objectDataTypeForPlainType);
			if (typeName.toString().contains("List")) {
				typeName.deleteCharAt(4);
				typeName.deleteCharAt(typeName.length-1);
			}
			nameStringBuilder.append(typeName.toString());
		}
		return nameStringBuilder.toString;
	}

	def getCommaSeperatedTypedOutputParameterList(
		Iterable<FArgument> arguments,
		boolean linebreak
	) {
		val returnStringBuilder = new StringBuilder();
		for (FArgument argument : arguments) {

			returnStringBuilder.append(argument.typeName);
			returnStringBuilder.append(" ");
			returnStringBuilder.append(argument.joynrName);
			returnStringBuilder.append(",");

			if (linebreak) {
				returnStringBuilder.append("\n");
			} else {
				returnStringBuilder.append(" ");
			}
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last " ," or "\n,"
		}
	}

	def getCommaSeperatedTypedOutputParameterList(FMethod method) {
		return getCommaSeperatedTypedOutputParameterList(getOutputParameters(method), false)
	}

	def getCommaSeperatedTypedOutputParameterList(FBroadcast broadcast) {
		return getCommaSeperatedTypedOutputParameterList(getOutputParameters(broadcast), false)
	}

	def getCommaSeperatedTypedOutputParameterListLinebreak(FBroadcast broadcast) {
		return getCommaSeperatedTypedOutputParameterList(getOutputParameters(broadcast), true)
	}

	def getCommaSeperatedUntypedInputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.inputParameters);
	}

	def getCommaSeperatedUntypedOutputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.outputParameters);
	}

	def getCommaSeperatedUntypedOutputParameterList(FBroadcast broadcast) {
		getCommaSeperatedUntypedParameterList(broadcast.outputParameters);
	}

	def getCommaSeperatedUntypedParameterList(Iterable<FArgument> arguments) {
		val returnStringBuilder = new StringBuilder();
		for (argument : arguments) {
			returnStringBuilder.append(argument.joynrName)
			returnStringBuilder.append(", ")
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		}
		return returnString.substring(0, returnString.length() - 2); //remove the last ,
	}

	def getCommaSeperatedTypedParameterList(FMethod method) {
		val returnStringBuilder = new StringBuilder();
		for (param : getInputParameters(method)) {
			returnStringBuilder.append(param.typeName);
			returnStringBuilder.append(" ");
			returnStringBuilder.append(param.joynrName);
			returnStringBuilder.append(", ");
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

	def getCommaSeperatedTypedFilterParameterList(FBroadcast broadcast) {
		val returnStringBuilder = new StringBuilder();
		for (filterParameter : getFilterParameters(broadcast)) {
			returnStringBuilder.append("String ");
			returnStringBuilder.append(filterParameter);
			returnStringBuilder.append(", ");
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

	override getTypeName(FBasicTypeId datatype) {
		switch datatype {
			case FBasicTypeId::BOOLEAN: "Boolean"
			case FBasicTypeId::INT8: "Byte"
			case FBasicTypeId::UINT8: "Byte"
			case FBasicTypeId::INT16: "Integer"
			case FBasicTypeId::UINT16: "Integer"
			case FBasicTypeId::INT32: "Integer"
			case FBasicTypeId::UINT32: "Integer"
			case FBasicTypeId::INT64: "Long"
			case FBasicTypeId::UINT64: "Long"
			case FBasicTypeId::FLOAT: "Double"
			case FBasicTypeId::DOUBLE: "Double"
			case FBasicTypeId::STRING: "String"
			case FBasicTypeId::BYTE_BUFFER: "byte[]"
			default: throw new IllegalArgumentException("Unsupported basic type: " + datatype.joynrName)
		}
	}

	def String getObjectDataTypeForPlainType(String plainType) {
		var type = plainType.toLowerCase
		switch (plainType) {
			case FBasicTypeId::BOOLEAN.getName: type = "Boolean"
			case FBasicTypeId::INT8.getName: type = "Byte"
			case FBasicTypeId::UINT8.getName: type = "Byte"
			case FBasicTypeId::INT16.getName: type = "Integer"
			case FBasicTypeId::UINT16.getName: type = "Integer"
			case FBasicTypeId::INT32.getName: type = "Integer"
			case FBasicTypeId::UINT32.getName: type = "Integer"
			case FBasicTypeId::INT64.getName: type = "Long"
			case FBasicTypeId::UINT64.getName: type = "Long"
			case FBasicTypeId::FLOAT.getName: type = "Double"
			case FBasicTypeId::DOUBLE.getName: type = "Double"
			case FBasicTypeId::STRING.getName: type = "String"
			case FBasicTypeId::BYTE_BUFFER.getName: type = "byte[]"
			case "void": type = "Void"
			default :  type = plainType
		}

		return type
	}
	override getTypeName(FType datatype) {
		datatype.joynrName
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		"List<" + getObjectDataTypeForPlainType(datatype.typeName) + ">";
	}

	override getTypeNameForList(FType datatype) {
		"List<" + getObjectDataTypeForPlainType(datatype.typeName) + ">";
	}

	def String getTypedParameterListJavaRpc(FMethod method) {
		var sb = new StringBuilder();
		val params = getInputParameters(method)
		var i = 0;
		while (i < params.size) {
			val param = params.get(i);
			sb.append("@JoynrRpcParam")
			sb.append("(\"" + param.joynrName + "\")")
			sb.append(" "+ param.typeName)
			sb.append(" "+ param.joynrName)
			if (i != params.size-1){
				sb.append(",\n")
			}
			i = i+1;
		}
		return sb.toString
	}

	def String getJavadocCommentsParameterListJavaRpc(FMethod method) {
		var sb = new StringBuilder();
		val params = getInputParameters(method)
		var i = 0;
		while (i < params.size) {
			val param = params.get(i);
			sb.append(" * @param " + param.joynrName + " the parameter " + param.joynrName + "\n");
			i = i+1;
		}
		return sb.toString
	}

	def String getTypedParameterListJavaTypeReference(FMethod method) {
		val sb = new StringBuilder()
		val params = getInputParameters(method)
		for (param : params) {
			sb.append("public static class "+param.typeName+ "Token extends TypeReference<"+param.typeName+" > {}\n")
		}
		sb.append("public static class "+ method.typeNamesForOutputParameter+ "Token extends TypeReference<"+ method.typeNamesForOutputParameter +" > {}\n")
		if (sb.length()==0) {
			return ""
		}
		return sb.toString
	}

	def String getTokenTypeForArrayType(String plainType) {
		if (plainType.contains("List<")) {
			return "List" + getObjectDataTypeForPlainType(plainType.substring(5, plainType.length-1));
		} else {
			return getObjectDataTypeForPlainType(plainType);
		}
	}

	def getDefaultValue(FTypedElement element) {
		getDefaultValue(element, "");
	}

	def getDefaultValue(FTypedElement element, String constructorParams) {
		//default values are not supported (currently) by the Franca IDL
/*		if (member.getDEFAULTVALUE()!=null && !member.getDEFAULTVALUE().isEmpty()){
			if (isEnum(member)){
				val ENUMDATATYPETYPE enumDatatype = getDatatype(id) as ENUMDATATYPETYPE
				for (ENUMELEMENTTYPE element : getEnumElements(enumDatatype)){
					if (element.VALUE == member.DEFAULTVALUE){
						return enumDatatype.SHORTNAME.toFirstUpper + "::" + element.SYNONYM
					}
				}
				return getPackagePath(enumDatatype, "::") + "::" + enumDatatype.SHORTNAME.toFirstUpper + "::" +  (enumDatatype.ENUMERATIONELEMENTS.ENUMELEMENT.get(0) as ENUMELEMENTTYPE).SYNONYM
			}
			else if (isLong(member.getDATATYPEREF().getIDREF())){
				return member.getDEFAULTVALUE() + "L"
			}
			else if (isDouble(member.getDATATYPEREF().getIDREF())){
				return member.getDEFAULTVALUE() + "d"
			}
			else{
				return member.getDEFAULTVALUE();
			}
		} else */ if (isComplex(element.type)) {
			if ((isArray(element))){
				return "new ArrayList<" + element.type.complexType.joynrName + ">(" + constructorParams + ")";
			} else {
				return "new " + element.type.complexType.joynrName + "(" + constructorParams + ")";
			}
		} else if (isEnum(element.type)) {
			if ((isArray(element))) {
				return "new ArrayList<" + element.type.enumType.joynrName + ">(" + constructorParams + ")";
			} else {
				return  element.type.enumType.joynrName + "." + element.type.enumType.enumerators.get(0).joynrName;
			}
		} else if (!primitiveDataTypeDefaultMap.containsKey(element.type.predefined)) {
 			return "NaN";
 		} else if (isPrimitive(element.type)) {
			if ((isArray(element))){
				return "new ArrayList<" + getPrimitive(element.type).typeName + ">(" + constructorParams + ")";
			} else {
				return primitiveDataTypeDefaultMap.get(element.type.predefined);
			}
		}
	}

	def String getJoynFullyQualifiedTypeName(FTypedElement typedElement) {
		if (typedElement.array == '[]') {
			return "List"
		}
		if (typedElement.type.derived != null) {
			getJoynFullyQualifiedTypeName(typedElement.type.derived)
		} else {
			typedElement.type.predefined.typeName
		}
	}

	def getJoynFullyQualifiedTypeName(FType type) {
		joynTypePackagePrefix + "." + type.typeName
	}
}