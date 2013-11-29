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
		«val typeName = type.joynrName»
		«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_")+"_"+typeName+"_H").toUpperCase»
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
		
		class «getDllExportMacro()» «typeName» : public «IF hasExtendsDeclaration(complexType)»«getExtendedType(complexType).joynrName»«ELSE»QObject«ENDIF»{
			Q_OBJECT

			
			«FOR member: getMembers(complexType)»
				«val membername = member.joynrName»
				«IF isArray(member)»
					Q_PROPERTY(QList<QVariant> «membername» READ get«membername.toFirstUpper»Internal WRITE set«membername.toFirstUpper»Internal)
				«ELSE»
					«IF isEnum(member.type)»
						Q_PROPERTY(QString «membername» READ get«membername.toFirstUpper»Internal WRITE set«membername.toFirstUpper»Internal)
					«ELSE»
«««						https://bugreports.qt-project.org/browse/QTBUG-2151 for why this replace is necessary
						Q_PROPERTY(«getMappedDatatypeOrList(member).replace("::","__")» «membername» READ get«membername.toFirstUpper» WRITE set«membername.toFirstUpper»)
					«ENDIF»
				«ENDIF»
			«ENDFOR»
	
		public:

			//general methods
			«typeName»();
			«typeName»(
				«FOR member: getMembersRecursive(complexType) SEPARATOR","»
					«getMappedDatatypeOrList(member)» «member.joynrName»
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
				«val joynrName = member.joynrName»
				«IF isArray(member)»
				 	QList<QVariant> get«joynrName.toFirstUpper»Internal() const;
				«ELSE»
					«IF isEnum(member.type)»
						QString get«joynrName.toFirstUpper»Internal() const;
					 «ENDIF»
				«ENDIF»
				«getMappedDatatypeOrList(member)» get«joynrName.toFirstUpper»() const;
			«ENDFOR»

			//setters
			«FOR member: getMembers(complexType)»
				«val joynrName = member.joynrName»
				«IF isArray(member)»
					void set«joynrName.toFirstUpper»Internal(const QList<QVariant>& «joynrName») ;
			 	«ELSE»
					«IF isEnum(member.type)»
						void set«joynrName.toFirstUpper»Internal(const QString& «joynrName») ;
					 «ENDIF»
				«ENDIF»
				void set«joynrName.toFirstUpper»(const «getMappedDatatypeOrList(member)»& «joynrName»);
			«ENDFOR»
			
		private:
			//members
			«FOR member: getMembers(complexType)»
				 «getMappedDatatypeOrList(member)» m_«member.joynrName»;
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
