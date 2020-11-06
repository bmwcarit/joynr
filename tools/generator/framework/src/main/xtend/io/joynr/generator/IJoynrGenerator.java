/*
 * #%L
 * io.joynr.tools.generator:generator-framework
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
package io.joynr.generator;

import java.util.Map;
import java.util.Set;

import org.eclipse.emf.ecore.resource.Resource;

import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.IGenerator;

import com.google.inject.Module;

public interface IJoynrGenerator extends IGenerator {

    public String getLanguageId();

    public void setParameters(Map<String, String> parameter);

    public Set<String> supportedParameters();

    public Module getGeneratorModule();

    public void updateCommunicationModelGeneration(Resource input);

    public void generateCommunicationModel(Resource input, IFileSystemAccess fsa);

    public void clearCommunicationModelGenerationSettings();
}
