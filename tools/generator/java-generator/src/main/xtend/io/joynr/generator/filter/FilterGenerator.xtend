package io.joynr.generator.filter
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
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface

class FilterGenerator {

	@Inject
	extension JoynrJavaGeneratorExtensions
	@Inject
	extension NamingUtil

	@Inject
	FilterTemplate filterTemplate;

	def doGenerate(FInterface fInterface, IFileSystemAccess fsa, boolean generateVersion) {
		val path = getPackagePathWithJoynrPrefix(fInterface, File::separator, generateVersion) + File::separator
		for (broadcast : fInterface.broadcasts) {
			if (broadcast.selective) {
				val fileName = path + fInterface.joynrName + broadcast.joynrName.toFirstUpper + "BroadcastFilter.java"
				generateFile(
					fsa,
					fileName,
					filterTemplate,
					fInterface,
					broadcast,
					generateVersion);
			}
		}
	}
}
