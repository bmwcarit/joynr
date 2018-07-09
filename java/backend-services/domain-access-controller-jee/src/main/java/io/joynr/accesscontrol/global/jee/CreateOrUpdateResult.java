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
package io.joynr.accesscontrol.global.jee;

import joynr.infrastructure.DacTypes.ChangeType;

public class CreateOrUpdateResult<T> {

    private final ChangeType changeType;
    private final T entry;

    public CreateOrUpdateResult(T entry, ChangeType changeType) {
        assert changeType == ChangeType.ADD
                || changeType == ChangeType.UPDATE : "CreateOrUpdateResult change type must be one of ADD or UPDATE";
        this.entry = entry;
        this.changeType = changeType;
    }

    public ChangeType getChangeType() {
        return changeType;
    }

    public T getEntry() {
        return entry;
    }
}
