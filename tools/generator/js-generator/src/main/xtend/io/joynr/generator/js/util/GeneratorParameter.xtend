package io.joynr.generator.js.util

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
import com.google.inject.Singleton
import io.joynr.generator.templates.util.NamingUtil
import java.util.Map
import java.util.Arrays
import java.util.HashSet
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType

@Singleton
class GeneratorParameter {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension NamingUtil

	Map<String, String> parameters;

	static final String requireJSSupportKey = "requireJSSupport";

	static final String requiredModuleKey = "requiredModule";

	static final String anonymuousDefineKey = "anonymuousDefine";

	static final String definePrefixKey = "definePrefix";

	static final String CONST_REQUIRED_MODULE_DEFAULT = "libjoynr";

	static final String CONST_DEFINE_PREFIX_DEFAULT = "";

	public boolean requireJSSupport = false;
	public String requiredModule;
	public boolean anonymuousDefine = true;
	public String definePrefix;

	def setParameters(Map<String, String> parameters) {
		this.parameters = parameters
		requireJSSupport = determRequireJsSupport
		requiredModule = determRequiredModule
		anonymuousDefine = determAnonymuousDefine
		definePrefix = determDefinePrefix
	}

	def supportedParameters() {
		new HashSet(Arrays.asList(requireJSSupportKey, requiredModuleKey, anonymuousDefineKey, definePrefixKey))
	}

	private def determRequireJsSupport(){
		parameters !== null && parameters.get(requireJSSupportKey) !== null && parameters.get(requireJSSupportKey).equalsIgnoreCase("true")
	}

	private def determRequiredModule(){
		if (parameters === null || parameters.get(requiredModuleKey) === null){
			return CONST_REQUIRED_MODULE_DEFAULT;
		} else {
			return parameters.get(requiredModuleKey);
		}
	}

	private def determDefinePrefix(){
		if (parameters === null || parameters.get(definePrefixKey) === null){
			return CONST_DEFINE_PREFIX_DEFAULT;
		} else {
			return parameters.get(definePrefixKey);
		}
	}

	private def determAnonymuousDefine() {
		parameters === null || parameters.get(anonymuousDefineKey) === null || !parameters.get(anonymuousDefineKey).equalsIgnoreCase("false")
	}

	def defineName(FModelElement element, boolean generateVersion){
		defineName(element, element.joynrName, generateVersion)
	}
	def defineName(FModelElement element, String moduleName, boolean generateVersion){
		if (anonymuousDefine){
			""
		} else if (element instanceof FType){
			"\"" + definePrefix + element.buildPackagePath("/", true, generateVersion) + "/" + moduleName + "\", "
		} else {
			"\"" + definePrefix + getPackagePathWithJoynrPrefix(element, "/", generateVersion) + "/" + moduleName + "\", "
		}
	}
}
