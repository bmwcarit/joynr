package io.joynr.capabilities;

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

/**
 * lOCALONLY: capability is registered locally, and is not to be forwarded to the global capdir
 * LOCALGLOBAL: capability is registered locally, and WILL be forwraded to the global capdir
 * REMOTE: capability is NOT registered locally, came from the global capdir
 *
 */
public enum CapabilityScope {
    LOCALONLY, LOCALGLOBAL, REMOTE, NOTSET
};