/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.capabilities;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import jakarta.persistence.CascadeType;
import jakarta.persistence.Column;
import jakarta.persistence.Embeddable;
import jakarta.persistence.OneToMany;

import joynr.types.CustomParameter;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@Embeddable
public class ProviderQosPersisted extends ProviderQos implements Serializable {
    private static final long serialVersionUID = 1L;
    private List<CustomParameterPersisted> customParameterList = new ArrayList<>();

    public ProviderQosPersisted() {
    }

    public ProviderQosPersisted(ProviderQos providerQos) {
        super(providerQos.getCustomParameters(),
              providerQos.getPriority(),
              providerQos.getScope(),
              providerQos.getSupportsOnChangeSubscriptions());
        for (CustomParameter customParameter : providerQos.getCustomParameters()) {
            customParameterList.add(new CustomParameterPersisted(customParameter));
        }
    }

    @OneToMany(cascade = { CascadeType.ALL }, orphanRemoval = true)
    public List<CustomParameterPersisted> getCustomParameterList() {
        return customParameterList;
    }

    public void setCustomParameterList(List<CustomParameterPersisted> customParameterList) {
        this.customParameterList = customParameterList;
        super.setCustomParameters(customParameterList.toArray(new CustomParameter[customParameterList.size()]));
    }

    @Override
    public Long getPriority() {
        return super.getPriority();
    }

    @Override
    public void setPriority(Long priority) {
        super.setPriority(priority);
    }

    @Override
    @Column(columnDefinition = "int4")
    public ProviderScope getScope() {
        return super.getScope();
    }

    @Override
    public void setScope(ProviderScope scope) {
        super.setScope(scope);
    }

    @Override
    public Boolean getSupportsOnChangeSubscriptions() {
        return super.getSupportsOnChangeSubscriptions();
    }

    @Override
    public void setSupportsOnChangeSubscriptions(Boolean supportsOnChangeSubscriptions) {
        super.setSupportsOnChangeSubscriptions(supportsOnChangeSubscriptions);
    }

    @Override
    public int hashCode() {
        int result = super.hashCode();
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        return true;
    }
}
