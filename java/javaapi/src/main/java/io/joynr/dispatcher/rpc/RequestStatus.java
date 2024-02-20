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
package io.joynr.dispatcher.rpc;

import java.util.ArrayList;
import java.util.List;

public class RequestStatus {

    private RequestStatusCode code;
    private List<String> descriptionList;

    public RequestStatus(RequestStatusCode code, String description) {
        super();
        this.code = code;
        this.descriptionList = new ArrayList<String>();
        descriptionList.add(description);
    }

    public RequestStatus(RequestStatusCode code) {
        super();
        this.code = code;
        this.descriptionList = new ArrayList<String>();
    }

    public RequestStatusCode getCode() {
        return code;
    }

    public void setCode(RequestStatusCode code) {
        this.code = code;
    }

    public List<String> getDescriptionList() {
        return (descriptionList != null) ? new ArrayList<>(descriptionList) : null;
    }

    public void setDescriptionList(List<String> description) {
        this.descriptionList = (description != null) ? new ArrayList<>(description) : null;
    }

    public void addDescription(String additionalDescription) {
        descriptionList.add(additionalDescription);
    }

    public boolean successful() {
        return code == RequestStatusCode.OK;
    }
}
