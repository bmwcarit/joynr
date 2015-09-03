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

import io.joynr.generator.util.JoynrGeneratorExtensions
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType

class JoynrJSGeneratorExtensions extends JoynrGeneratorExtensions {

	override getOneLineWarning() {
		throw new UnsupportedOperationException("Auto-generated function stub")
	}

	def getAttributeCaps(FAttribute attribute)
	'''«IF isNotifiable(attribute)»Notify«ENDIF»«IF isReadable(attribute)»Read«ENDIF»«IF isWritable(attribute)»Write«ENDIF»'''

	def getFQN(FInterface fInterface) {
		getPackagePathWithoutJoynrPrefix(fInterface, "/") + "/" + fInterface.joynrName.toLowerCase
	}

	//TODO: refactor this for general use by all languages. Requires normalising how separators are added
	def buildPackagePath(FType datatype, String separator, boolean includeTypeCollection) {
		if (datatype == null) {
			return "";
		}
		var packagepath = "";
		try {
			packagepath = getPackagePathWithJoynrPrefix(datatype, separator);
		} catch (IllegalStateException e){
			//	if an illegal StateException has been thrown, we tried to get the package for a primitive type, so the packagepath stays empty.
		}
		if (packagepath!="") {
			if (includeTypeCollection && datatype.partOfTypeCollection) {
				packagepath = packagepath + separator + datatype.typeCollectionName.toLowerCase;
			}
		};
		return packagepath;
	}
}
