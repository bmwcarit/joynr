package io.joynr.generator.js.templates
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

import io.joynr.generator.templates.InterfaceTemplate
import java.io.File
import java.util.regex.Pattern

/*
 * This is the base class used by all generation templates which process a
 * Franca interface type
 */
abstract class InterfaceJsTemplate extends InterfaceTemplate {
	/*
	 * the folder path of the interface being generated, starting from
	 * the defined output path when invoking the generator
	 */
	protected String path

	/*
	 * the folder path depth of the interface being generated, starting from
	 * the defined output path when invoking the generator
	 */
	protected int packagePathDepth

	override generate(boolean generateVersion) {
		val packagePathWithJoynrPrefix = getPackagePathWithJoynrPrefix(francaIntf, File::separator, generateVersion)
		path = File::separator + packagePathWithJoynrPrefix + File::separator
		packagePathDepth = packagePathWithJoynrPrefix.split(Pattern.quote(File::separator)).length
		'''
		'''
	}

}
