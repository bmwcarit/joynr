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

import com.google.inject.Inject
import com.google.inject.Singleton
import java.util.ArrayList
import java.util.HashSet
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface

@Singleton
public class BroadcastUtil {

	@Inject
	private extension TypeUtil

	@Inject
	private extension NamingUtil

	def Iterable<FArgument> getOutputParameters(FBroadcast event) {
		if (event == null || event.outArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			return event.outArgs.filterNull
		}
	}

	def getAllRequiredTypes(FBroadcast broadcast) {
		var typeList = new HashSet<Object>();
		for (outParameter : broadcast.outputParameters.filterNull) {
			typeList.addAll(getRequiredTypes(outParameter.type));
		}
		return typeList
	}

	def getAllComplexTypes(FBroadcast broadcast) {
		broadcast.allRequiredTypes.filterComplex
	}

	def getFilterParameters(FBroadcast broadcast) {
		val paramList = new ArrayList<String>();
		if (broadcast.comment != null) {
			for(annotation: broadcast.comment.elements) {
				if(annotation.type == FAnnotationType::PARAM) {
					val comment = annotation.comment
					paramList.add(comment.split("\\s+").get(0))
				}
			}
		}
		return paramList
	}

	def getBroadcastFilterClassName(FInterface francaIntf, FBroadcast broadcast) {
		val broadcastName = broadcast.joynrName;
		val broadCastFilterClassName = francaIntf.joynrName.toFirstUpper + broadcastName.toFirstUpper + "BroadcastFilter";
		return broadCastFilterClassName
	}
}
