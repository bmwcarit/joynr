package io.joynr.generator.cpp.provider
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
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class InterfaceRequestInterpreterCppTemplate {

	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions

	def generate(FInterface serviceInterface){
		val interfaceName = serviceInterface.joynrName
		'''
		«warning()»
		
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"
		
		#include "joynr/Request.h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"
		#include "joynr/DeclareMetatypeUtil.h"
		#include "joynr/Util.h"
		#include "joynr/RequestStatus.h"
		#include <cassert>
		
		«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
			#include "«parameterType»"
		«ENDFOR»
		
		«getNamespaceStarter(serviceInterface)» 
		
		joynr::joynr_logging::Logger* «interfaceName»RequestInterpreter::logger = joynr::joynr_logging::Logging::getInstance()->getLogger("SDMO", "«interfaceName»JsonRequestInterpreter");
		
		«interfaceName»RequestInterpreter::«interfaceName»RequestInterpreter()
		{
			«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
				«IF datatype instanceof FType»
					qRegisterMetaType<«getMappedDatatype(datatype as FType)»>("«getMappedDatatype(datatype as FType)»");
				«ENDIF»
			«ENDFOR»
		}
		
		QVariant «interfaceName»RequestInterpreter::execute(
		        QSharedPointer<joynr::RequestCaller> requestCaller,
		        const QString& methodName,
		        const QList<QVariant>& paramValues,
		        const QList<QVariant>& paramTypes)
		{
			Q_UNUSED(paramValues);//if all methods of the interface are empty, the paramValues would not be used and give a warning.
			Q_UNUSED(paramTypes);//if all methods of the interface are empty, the paramTypes would not be used and give a warning.
			// cast generic RequestCaller to «interfaceName»Requestcaller
			QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.requestCaller» =
					requestCaller.dynamicCast<«interfaceName»RequestCaller>();
			
			joynr::RequestStatus status;
			// execute operation
			// TODO need to put the status code into the reply
			«val attributes = getAttributes(serviceInterface)»
			«val methods = getMethods(serviceInterface)»
			«IF attributes.size>0»
				«FOR attribute: attributes SEPARATOR "\n} else"»
					«val attributeName = attribute.joynrName»
					if (methodName == "get«attributeName.toFirstUpper»"){
						«getMappedDatatypeOrList(attribute)» returnValue;
						«serviceInterface.requestCaller»->get«attributeName.toFirstUpper»(status, returnValue);
						// convert typed return value into variant
						«IF isArray(attribute)»
							return joynr::Util::convertListToVariantList(returnValue);
						«ELSE»
							return QVariant::fromValue(returnValue);
						«ENDIF»
					} else if (methodName == "set«attributeName.toFirstUpper»" && paramTypes.size() == 1){
						QVariant «attributeName»QVar = paramValues.at(0);
						«IF isEnum(attribute.type)»
							«getMappedDatatypeOrList(attribute)» typedInput«attributeName.toFirstUpper» = joynr::Util::convertVariantToEnum<«getEnumContainer(attribute.type)»>(«attributeName»QVar);
						«ELSEIF isEnum(attribute.type) && isArray(attribute)»
							«getMappedDatatypeOrList(attribute)» typedInput«attributeName.toFirstUpper» =
								joynr::Util::convertVariantListToEnumList<«getEnumContainer(attribute.type)»>(«attributeName»QVar.toList());
						«ELSEIF isArray(attribute)»
							assert(«attributeName»QVar.canConvert<QList<QVariant> >());
							QList<QVariant> paramQList = «attributeName»QVar.value<QList<QVariant> >();
							«getMappedDatatypeOrList(attribute)» typedInput«attributeName.toFirstUpper» = joynr::Util::convertVariantListToList<«getMappedDatatype(attribute)»>(paramQList);
						«ELSE»
							assert(«attributeName»QVar.canConvert<«getMappedDatatypeOrList(attribute)»>());
							«getMappedDatatypeOrList(attribute)» typedInput«attributeName.toFirstUpper» = «attributeName»QVar.value<«getMappedDatatypeOrList(attribute)»>();
						«ENDIF»
						«serviceInterface.requestCaller»->set«attributeName.toFirstUpper»(status, typedInput«attributeName.toFirstUpper»);
						QVariant returnValue("void");
						return returnValue;

				«ENDFOR»
				}«IF methods.size>0» else«ENDIF»
			«ENDIF»
			«IF methods.size>0»
				«FOR method: getMethods(serviceInterface) SEPARATOR "\n} else"»
					«val outputParameterType = getMappedOutputParameter(method).head»
					«val inputParameterList = getCommaSeperatedTypedParameterList(method)»
					  
					«val methodName = method.joynrName»
					«val inputParams = getInputParameters(method)»
					«var iterator = -1»
					if(methodName == "«methodName»" && paramTypes.size() == «inputParams.size»
						«FOR input : inputParams»
						&& paramTypes.at(«iterator=iterator+1») == "«getJoynrTypeName(input)»"
						«ENDFOR»
					) {
						«IF outputParameterType != "void" && inputParameterList == ""»
							«getMappedOutputParameter(method).head» typedReturnValue;
							«serviceInterface.requestCaller»->«methodName»(status, typedReturnValue);
							«IF isArray(getOutputParameters(method).head)»
								QList<QVariant> returnValue = joynr::Util::convertListToVariantList<«getMappedDatatype( getOutputParameters(method).head)»>(typedReturnValue);
								return returnValue;
							«ELSE»
								return QVariant::fromValue(typedReturnValue);
							«ENDIF»
							
						«ELSE»
							«var iterator2 = -1»
							«FOR input : inputParams»
								«val inputName = input.joynrName»
								QVariant «inputName»QVar = paramValues.at(«iterator2=iterator2+1»);
								«IF isEnum(input.type) && isArray(input)»
									//isEnumArray
									«getMappedDatatypeOrList(input)» typedInput«inputName.toFirstUpper» = 
										joynr::Util::convertVariantListToEnumList<«getEnumContainer(input.type)»> («inputName»QVar.toList());
								«ELSEIF isEnum(input.type)»
									//isEnum
									«getMappedDatatypeOrList(input)» typedInput«inputName.toFirstUpper» = joynr::Util::convertVariantToEnum<«getEnumContainer(input.type)»>(«inputName»QVar);
								«ELSEIF isArray(input)»
									//isArray
									assert(«inputName»QVar.canConvert<QList<QVariant> >());
									QList<QVariant> «inputName»QVarList = «inputName»QVar.value<QList<QVariant> >();
									QList<«getMappedDatatype(input)»> typedInput«inputName.toFirstUpper» = joynr::Util::convertVariantListToList<«getMappedDatatype(input)»>(«inputName»QVarList);
								«ELSE»
									//«getMappedDatatypeOrList(input)»
									assert(«inputName»QVar.canConvert<«getMappedDatatypeOrList(input)»>());
									«getMappedDatatypeOrList(input)» typedInput«inputName.toFirstUpper» = «inputName»QVar.value<«getMappedDatatypeOrList(input)»>();
								«ENDIF»
							«ENDFOR»
							«IF outputParameterType != "void"»
								«getMappedOutputParameter(method).head» typedReturnValue;
								«serviceInterface.requestCaller»->«methodName»(
									status, 
									typedReturnValue«IF inputParams.size>0»,«ENDIF» 
							«ELSE»
								«serviceInterface.requestCaller»->«methodName»(
									status«IF inputParams.size>0»,«ENDIF» 
							«ENDIF»		
								«FOR input : inputParams SEPARATOR ','»
									typedInput«input.joynrName.toFirstUpper»
								«ENDFOR» 
							);

							«IF outputParameterType == "void"»
								QVariant returnValue("void");
								return returnValue;
							«ELSEIF isArray(getOutputParameters(method).head) && isEnum(getOutputParameters(method).head.type)»
								QList<QVariant> returnValue = joynr::Util::convertListToVariantList<«getMappedDatatype( getOutputParameters(method).head)»>(typedReturnValue);
								return returnValue;
							«ELSEIF isArray(getOutputParameters(method).head)»
								QList<QVariant> returnValue = joynr::Util::convertListToVariantList<«getMappedDatatype( getOutputParameters(method).head)»>(typedReturnValue);
								return returnValue;
							«ELSEIF isEnum(getOutputParameters(method).head.type)»
								return QVariant::fromValue(typedReturnValue);
							«ELSE»
								return QVariant::fromValue(typedReturnValue);
							«ENDIF»		
						«ENDIF»
				«ENDFOR»
				}
			«ENDIF»
			LOG_FATAL(logger, "unknown method name for interface «interfaceName»: " + methodName);
			assert(false);
			return QVariant();
		}
		
		«getNamespaceEnder(serviceInterface)» 
		'''
	}
	
	def getRequestCaller(FInterface serviceInterface){
	    serviceInterface.joynrName.toFirstLower+"RequestCaller"
	}
	
}

