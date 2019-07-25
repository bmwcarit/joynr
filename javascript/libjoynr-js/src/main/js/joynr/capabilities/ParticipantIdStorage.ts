/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import { Persistency } from "../../global/interface/Persistency";
import { JoynrProvider, JoynrProviderType } from "../types/JoynrProvider";
import * as CapabilitiesUtil from "../util/CapabilitiesUtil";
import nanoid from "nanoid";
type nanoidType = typeof nanoid;

class ParticipantIdStorage {
    private nanoid: nanoidType;
    private persistency: Persistency;
    /**
     * @constructor
     *
     * @param persistency - the persistence object to be used to store the participantIds
     * @param nanoid - a function generating a unique string
     */
    public constructor(persistency: Persistency, nanoid: nanoidType) {
        this.persistency = persistency;
        this.nanoid = nanoid;
    }

    /**
     * @param domain
     * @param provider
     * @param provider.interfaceName
     *
     * @returns the retrieved or generated participantId
     */
    public getParticipantId(domain: string, provider: JoynrProvider): string {
        const key = CapabilitiesUtil.generateParticipantIdStorageKey(
            domain,
            provider.interfaceName,
            ((provider.constructor as any) as JoynrProviderType).MAJOR_VERSION
        );
        let participantId = this.persistency.getItem(key);
        if (!participantId) {
            participantId = this.nanoid();
            this.persistency.setItem(key, participantId);
        }
        return participantId;
    }

    /**
     * @param domain
     * @param provider
     * @param provider.interfaceName
     * @param participantId
     */
    public setParticipantId(domain: string, provider: JoynrProvider, participantId: string): void {
        const key = CapabilitiesUtil.generateParticipantIdStorageKey(
            domain,
            provider.interfaceName,
            ((provider.constructor as any) as JoynrProviderType).MAJOR_VERSION
        );
        this.persistency.setItem(key, participantId);
    }
}

export = ParticipantIdStorage;
