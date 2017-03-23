package io.joynr.generator.templates.util
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

import com.google.inject.Singleton
import org.franca.core.franca.FAttribute

@Singleton
public class AttributeUtil {


	def isReadable(FAttribute field) {
		return true
	}

	def boolean isWritable(FAttribute field) {
		return !field.isReadonly
	}

	def isNotifiable(FAttribute field) {
		return field.observable
	}

	def boolean isReadonly(FAttribute fAttribute) {
		fAttribute.readonly
	}

	def boolean isObservable(FAttribute fAttribute) {
		!fAttribute.noSubscriptions
	}}