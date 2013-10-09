package io.joynr.generator.cpp.communicationmodel
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
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class TypeCppTemplate {

	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions
	
	def generate(FType type) {
		val typeName = type.name.toFirstUpper
		'''
		«warning»		
		
		#include "«getPackagePathWithJoynrPrefix(type, "/")»/«typeName».h"
		#include "joynr/Reply.h"
		#include "joynr/DeclareMetatypeUtil.h"
		#include "qjson/serializer.h"
		#include <QMetaEnum>
		#include <QDateTime>

		
		«getNamespaceStarter(type)»
		
		void «typeName»::registerMetatypes(){ 
			qRegisterMetaType<«getMappedDatatype(type)»>("«getMappedDatatype(type)»");
			«FOR complexMember: getComplexMembers(type as FCompoundType)»
				qRegisterMetaType<«getMappedDatatype(complexMember)»>("«getMappedDatatype(complexMember)»");
				qRegisterMetaType<«getMappedDatatype(complexMember).replace('::','__')»>("«getMappedDatatype(complexMember).replace('::','__')»");
			«ENDFOR»
			«FOR enumMember: getEnumMembers(type as FCompoundType)»
				{
					qRegisterMetaType<«getEnumContainer(enumMember.type.derived)»>();
					int id = qRegisterMetaType<«getMappedDatatype(enumMember)»>();
					QJson::Serializer::registerEnum(id, «getEnumContainer(enumMember.type.derived)»::staticMetaObject.enumerator(0));
				}
			«ENDFOR»
		}

		«typeName»::«typeName»():
			«IF hasExtendsDeclaration(type as FCompoundType)»
				«getExtendedType(type as FCompoundType).name»(), 
			«ENDIF»
			«FOR member: getMembers(type as FCompoundType) SEPARATOR ','» 
				m_«member.name»(«getDefaultValue(member)»)
			«ENDFOR»
		{
			registerMetatypes();
		}
		
		«typeName»::«typeName»(
			«FOR member: getMembersRecursive(type) SEPARATOR ','»
			 	«getMappedDatatypeOrList(member)» new_«member.name»
		 	«ENDFOR»
		 	 ):
			«IF hasExtendsDeclaration(type as FCompoundType)»
				«val extendedType = getExtendedType(type as FCompoundType)»
				«extendedType.name»(
				«FOR member: getMembersRecursive(extendedType) SEPARATOR ','»
					new_«member.name»
				«ENDFOR»
				),
			«ENDIF»
			«FOR member: getMembers(type as FCompoundType) SEPARATOR ','» 
				m_«member.name»(new_«member.name») 
			«ENDFOR»
		{
			registerMetatypes();
		}
		
		
		//CopyConstructor
		«typeName»::«typeName»(const «typeName»& «typeName.toFirstLower»Obj) :
			«IF hasExtendsDeclaration(type as FCompoundType)»
				«getExtendedType(type as FCompoundType).name»(«typeName.toFirstLower»Obj), 
			«ELSE»
				QObject(),
			«ENDIF»
			«FOR member: getMembers(type as FCompoundType) SEPARATOR ','»
				m_«member.name»(«typeName.toFirstLower»Obj.m_«member.name»)
			«ENDFOR»
		{
			registerMetatypes();		
		}
		
		«typeName»::~«typeName»(){
		}
		
		«FOR member: getMembers(type as FCompoundType)»
			«val memberName = member.name»
			«IF isArray(member)»
				QList<QVariant> «typeName»::get«memberName.toFirstUpper»Internal() const {
					QList<QVariant> returnList;
					returnList.reserve( this->m_«memberName».size() );
				    «getMappedDatatypeOrList(member)»::const_iterator iter = this->m_«memberName».begin();
				    while(iter!=this->m_«memberName».end()){
				        QVariant value;
				    	«IF isEnum(member.type)»
				    		value.setValue((int)*iter);
				    	«ELSE»
				    		value.setValue(*iter);
				    	«ENDIF»
				        returnList.push_back(value);
				        iter++;
				    }
					return returnList;
				}
				
				void «typeName»::set«memberName.toFirstUpper»Internal(const QList<QVariant>& obj«memberName»)  {
					this->m_«memberName».clear();
					this->m_«memberName».reserve( obj«memberName».size() );
				    QList<QVariant>::const_iterator iter = obj«memberName».begin();
				    while(iter!=obj«memberName».end()){
				    	«IF isEnum(member.type)»
				    		this->m_«memberName».push_back((«getMappedDatatype(member)»)(*iter).value<int>());
				    	«ELSE»
				    		this->m_«memberName».push_back((*iter).value<«getMappedDatatype(member)»>());
				    	«ENDIF»
				        iter++;
				    }
				}
				
			«ELSE»
				«IF isEnum(member.type)»
					QString «typeName»::get«memberName.toFirstUpper»Internal() const {
						QMetaEnum metaEnum = «getMappedDatatypeOrList(member).substring(0, getMappedDatatypeOrList(member).length-6)»::staticMetaObject.enumerator(0);
						return metaEnum.valueToKey(this->m_«memberName»);
					}
					
					void «typeName»::set«memberName.toFirstUpper»Internal(const QString& obj«memberName»)  {
						QMetaEnum metaEnum = «getMappedDatatypeOrList(member).substring(0, getMappedDatatypeOrList(member).length-6)»::staticMetaObject.enumerator(0);
						int value = metaEnum.keyToValue(obj«memberName».toStdString().c_str());
						this->m_«memberName» = («getMappedDatatypeOrList(member)»)value;
					}
					
				«ENDIF»
			«ENDIF»
			«getMappedDatatypeOrList(member)» «typeName»::get«memberName.toFirstUpper»() const {
				return m_«memberName»;  
			}
			
			void «typeName»::set«memberName.toFirstUpper»(const «getMappedDatatypeOrList(member)»& obj«memberName»){
				this->m_«memberName» = obj«memberName»;
			}
		«ENDFOR»
		
		//AssignOperator
		«typeName»& «typeName»::operator=(const «typeName»& «typeName.toFirstLower»Obj) {
			«IF hasExtendsDeclaration(type as FCompoundType)»
				«val base = getExtendedType(type as FCompoundType)»
				«getMappedDatatype(base)»::operator=(«typeName.toFirstLower»Obj); 
			«ENDIF»
			«FOR member: getMembers(type as FCompoundType)»
				this->m_«member.name» = «typeName.toFirstLower»Obj.m_«member.name»;
			«ENDFOR»
		    return *this;
		}
		
		bool «typeName»::operator==(const «typeName»& «typeName.toFirstLower»Obj) const {
			return
			    «FOR member: getMembers(type as FCompoundType)»
			    	this->m_«member.name» == «typeName.toFirstLower»Obj.m_«member.name» &&
				«ENDFOR»
				«IF hasExtendsDeclaration(type as FCompoundType)»
					«getMappedDatatype(getExtendedType(type as FCompoundType))»::operator==(«typeName.toFirstLower»Obj);
				«ELSE»
					true;
				«ENDIF»
		}

		bool «typeName»::operator!=(const «typeName»& «typeName.toFirstLower»Obj) const {
		    return !(*this==«typeName.toFirstLower»Obj);
		}
		
		QString «typeName»::toString() const {
		    QString result;
			«IF hasExtendsDeclaration(type as FCompoundType)»
				result += «getMappedDatatype(type)»::toString();
			«ENDIF»
		    «FOR member: getMembers(type as FCompoundType)»
		    	«val memberName = member.name»
		    	«IF isArray(member)»
		    		result += " unprinted List «memberName»  ";
				«ELSEIF isEnum(member.type)»
		    		result += "«memberName» :" + get«memberName.toFirstUpper»Internal();
				«ELSEIF isComplex(member.type)»
		    		result += "«memberName» :" + get«memberName.toFirstUpper»().toString();
				«ELSE»
		    		result += "«memberName» :(casted to String via Qs)" + QVariant(get«memberName.toFirstUpper»()).toString(); 
				«ENDIF»
			«ENDFOR»
			return result;
		}
		«getNamespaceEnder(type)»
		
		'''
	}		
}
