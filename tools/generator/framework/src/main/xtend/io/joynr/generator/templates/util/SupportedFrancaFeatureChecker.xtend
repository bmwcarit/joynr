package io.joynr.generator.templates.util

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
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
 * #L%
 */
import org.franca.core.franca.FModel
import org.eclipse.xtext.EcoreUtil2
import org.franca.core.franca.FTypeRef

class SupportedFrancaFeatureChecker {
	def static checkModel(FModel model) {
		checkIntegerDatatypeIsNotUsed(model)
	}

	def private static checkIntegerDatatypeIsNotUsed(FModel model) {
		val allTypeRefs = EcoreUtil2::getAllContentsOfType(model, typeof(FTypeRef))

		if (allTypeRefs.exists[it.interval != null]) {
			throw new Exception("Franca's datatype 'Integer' is not supported. Use Int8, Int16, Int32 or Int64 instead")
		}
	}
}
