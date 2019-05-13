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
const CapabilitiesUtil = require("../util/CapabilitiesUtil");
/**
 * @constructor
 * @name ParticipantIdStorage
 *
 * @param {Persistency} persistency - the persistence object to be used to store the participantIds
 * @param {Function} nanoid - a function generating a unique string
 */
function ParticipantIdStorage(persistency, nanoid) {
    this._persistency = persistency;
    this._nanoid = nanoid;
}

/**
 * @function
 * @name ParticipantIdStorage#getParticipantId
 *
 * @param {String} domain
 * @param {Object} provider
 * @param {String} provider.interfaceName
 *
 * @returns {String} the retrieved or generated participantId
 */
ParticipantIdStorage.prototype.getParticipantId = function getParticipantId(domain, provider) {
    const key = CapabilitiesUtil.generateParticipantIdStorageKey(
        domain,
        provider.interfaceName,
        provider.constructor.MAJOR_VERSION
    );
    let participantId = this._persistency.getItem(key);
    if (!participantId) {
        participantId = this._nanoid();
        this._persistency.setItem(key, participantId);
    }
    return participantId;
};

/**
 * @function
 * @name ParticipantIdStorage#setParticipantId
 *
 * @param {String} domain
 * @param {Object} provider
 * @param {String} provider.interfaceName
 * @param {String} participantId
 */
ParticipantIdStorage.prototype.setParticipantId = function setParticipantId(domain, provider, participantId) {
    const key = CapabilitiesUtil.generateParticipantIdStorageKey(
        domain,
        provider.interfaceName,
        provider.constructor.MAJOR_VERSION
    );
    this._persistency.setItem(key, participantId);
};

module.exports = ParticipantIdStorage;
