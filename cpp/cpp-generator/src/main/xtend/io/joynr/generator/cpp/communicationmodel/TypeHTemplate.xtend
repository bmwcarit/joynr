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

import io.joynr.generator.cpp.util.TemplateBase
import javax.inject.Inject
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class TypeHTemplate {

	@Inject
	private extension JoynrCppGeneratorExtensions
	
	@Inject
	private extension TemplateBase
	
	def generate(FType type)
	'''
		«val complexType = type as FCompoundType»
		«val typeName = type.name.toFirstUpper»
		«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_")+"_"+type.name+"_H").toUpperCase»
		«warning()»
		#ifndef «headerGuard»
		#define «headerGuard»
		
		«getDllExportIncludeStatement()»
		
		#include <QObject>
		#include <QVariantMap>
		#include <QList>


		//include complex Datatype headers. 		
		«FOR member: getRequiredIncludesFor(complexType)»
			#include "«member»"
		«ENDFOR»

		«getNamespaceStarter(type)»
		
		class «getDllExportMacro()» «typeName» : public «IF hasExtendsDeclaration(complexType)»«getExtendedType(complexType).name»«ELSE»QObject«ENDIF»{
			Q_OBJECT

			
			«FOR member: getMembers(complexType)»
				«val membername = member.name»
				«val Membername = member.name.toFirstUpper»
				«IF isArray(member)»
					Q_PROPERTY(QList<QVariant> «membername» READ get«Membername»Internal WRITE set«Membername»Internal)
				«ELSE»
					«IF isEnum(member.type)»
						Q_PROPERTY(QString «membername» READ get«Membername»Internal WRITE set«Membername»Internal)
					«ELSE»
«««						https://bugreports.qt-project.org/browse/QTBUG-2151 for why this replace is necessary
						Q_PROPERTY(«getMappedDatatypeOrList(member).replace("::","__")» «membername» READ get«Membername» WRITE set«Membername»)
					«ENDIF»
				«ENDIF»
			«ENDFOR»
	
		public:

			//general methods
			«typeName»();
			«typeName»(
				«FOR member: getMembersRecursive(complexType) SEPARATOR","»
					«getMappedDatatypeOrList(member)» «member.name»
				«ENDFOR»
			);
			«typeName»(const «typeName»& «typeName.toFirstLower»Obj);

			virtual ~«typeName»();

			QString toString() const;
			«typeName»& operator=(const «typeName»& «typeName.toFirstLower»Obj);
			bool operator==(const «typeName»& «typeName.toFirstLower»Obj) const;
			bool operator!=(const «typeName»& «typeName.toFirstLower»Obj) const;

			//getters
			«FOR member: getMembers(complexType)»
				«val memberName = member.name.toFirstUpper»
				«IF isArray(member)»
				 	QList<QVariant> get«memberName»Internal() const;
				«ELSE»
					«IF isEnum(member.type)»
						QString get«memberName»Internal() const;
					 «ENDIF»
				«ENDIF»
				«getMappedDatatypeOrList(member)» get«memberName»() const;
			«ENDFOR»

			//setters
			«FOR member: getMembers(complexType)»
				«val Membername = member.name.toFirstUpper»
				«val membername = member.name»
				«IF isArray(member)»
					void set«Membername»Internal(const QList<QVariant>& «membername») ;
			 	«ELSE»
					«IF isEnum(member.type)»
						void set«Membername»Internal(const QString& «membername») ;
					 «ENDIF»
				«ENDIF»
				void set«Membername»(const «getMappedDatatypeOrList(member)»& «membername»);
			«ENDFOR»
			
		private:
			//members
			«FOR member: getMembers(complexType)»
				 «getMappedDatatypeOrList(member)» m_«member.name»;
			«ENDFOR»
			void registerMetatypes();
			
		};	

		«getNamespaceEnder(type)»
«««		https://bugreports.qt-project.org/browse/QTBUG-2151 for why this typedef is necessary
		typedef «getPackagePathWithJoynrPrefix(type, "::")»::«typeName» «getPackagePathWithJoynrPrefix(type, "__")»__«typeName»;
		Q_DECLARE_METATYPE(«getPackagePathWithJoynrPrefix(type, "__")»__«typeName»)
		Q_DECLARE_METATYPE(QList<«getPackagePathWithJoynrPrefix(type, "__")»__«typeName»>)
		
		#endif // «headerGuard»
	'''

}
