package io.joynr.generator.communicationmodel
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
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FCompoundType
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import org.franca.core.franca.FField

class ComplexTypeTemplate {

	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase
	
	def generate(FCompoundType complexType) {
		val typeName = complexType.joynrName
		val complexTypePackageName = getPackagePathWithJoynrPrefix(complexType, ".")
		'''
		«warning()»
		
package «complexTypePackageName»;
import java.io.Serializable;

import io.joynr.subtypes.JoynrType;

«FOR member : getRequiredIncludesFor(complexType)»
import «member»;
«ENDFOR»
«IF hasArrayMembers(complexType)»
import java.util.List;
«ENDIF»
«IF hasListsInConstructor(complexType)»
import java.util.ArrayList;
import com.google.common.collect.Lists;
«ENDIF»

@SuppressWarnings("serial")
// NOTE: serialVersionUID is not defined since we don't have versioning in BMWIDL right now. 
//       The compiler will generate a serialVersionUID based on the class and its members
//       (cf. http://docs.oracle.com/javase/6/docs/platform/serialization/spec/class.html#4100),
//       which is probably more restrictive than what we want.
public class «typeName»«IF hasExtendsDeclaration(complexType)» extends «getMappedDatatype(complexType.extendedType)»«ENDIF» implements Serializable, JoynrType {
	«FOR member : getMembers(complexType)»
	«val memberType = getMappedDatatypeOrList(member).replace("::","__")»
	«IF isArray(member)»
	private «memberType» «member.joynrName» = Lists.newArrayList();
	«ELSE»
	private «memberType» «member.joynrName»;
	«ENDIF»
	«ENDFOR»
	
	public «typeName»() {
		«FOR member : getMembers(complexType)»
		this.«member.joynrName» = «getDefaultValue(member)»;
		«ENDFOR»
	}
	

	public «typeName»(
		«FOR member : getMembersRecursive(complexType) SEPARATOR ','»
		«getMappedDatatypeOrList(member).replace("::","__")» «member.joynrName»
		«ENDFOR»
		) {
		«IF hasExtendsDeclaration(complexType)»
			super(
					«FOR member: getMembersRecursive(complexType.extendedType) SEPARATOR ','»
						«member.joynrName»
					«ENDFOR»
			);
		«ENDIF»
		«FOR member : getMembers(complexType)»
		this.«member.joynrName» = «member.joynrName»;
		«ENDFOR»
	}
	

	«FOR member : getMembers(complexType)»
	«val memberType = getMappedDatatypeOrList(member).replace("::","__")»
	«val memberName = member.joynrName»
	public «memberType» get«memberName.toFirstUpper»() {
		return this.«member.joynrName»;
	}
	
	public void set«memberName.toFirstUpper»(«memberType» «member.joynrName») {
		this.«member.joynrName» = «member.joynrName»;
	}
	
	«ENDFOR»
	
	@Override
	public String toString() {
		return "«typeName» ["
		«IF hasExtendsDeclaration(complexType)»
				+ super.toString() + ", "
		«ENDIF»
		«FOR member : getMembers(complexType) SEPARATOR " + \", \""»
			+ "«member.joynrName»=" + this.«member.joynrName»
		«ENDFOR»
		+ "]";
	}
	
	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		«IF hasExtendsDeclaration(complexType)»
			if (!super.equals(obj))
				return false;
		«ENDIF»
		«typeName» other = («typeName») obj;
		«FOR member : getMembers(complexType)»
			if (this.«member.joynrName» == null) {
				if (other.«member.joynrName» != null) {
					return false;
				}
			} else if (!this.«member.joynrName».equals(other.«member.joynrName»)){
				return false;
			}
		«ENDFOR»
		return true;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		«IF hasExtendsDeclaration(complexType)»
			int _result = super.hashCode();
		«ELSE»
			int _result = 1;
		«ENDIF»
		«FOR member : getMembers(complexType)»
			_result = prime * _result + ((this.«member.joynrName» == null) ? 0 : this.«member.joynrName».hashCode());
		«ENDFOR»
		return _result;
	}
}

	
		'''	
	}
}
