/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
extern crate serde;

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MasterAccessControlEntry {
    #[serde(rename = "_typeName")]
    type_name: String,
    uid: String,
    domain: String,
    interface_name: String,
    default_required_trust_level: String,
    possible_required_trust_levels: Vec<String>,
    default_required_control_entry_change_trust_level: String,
    possible_required_control_entry_change_trust_levels: Vec<String>,
    operation: String,
    default_consumer_permission: String,
    possible_consumer_permissions: Vec<String>,
}

impl MasterAccessControlEntry {
    pub fn new(
        in_uid: &String,
        in_domain: &String,
        in_interface_name: &String,
    ) -> MasterAccessControlEntry {
        MasterAccessControlEntry {
            type_name: "joynr.infrastructure.DacTypes.MasterAccessControlEntry".to_string(),
            uid: in_uid.to_string(),
            domain: in_domain.to_string(),
            interface_name: in_interface_name.to_string(),
            default_required_trust_level: "HIGH".to_string(),
            possible_required_trust_levels: vec!["HIGH".to_string()],
            default_required_control_entry_change_trust_level: "HIGH".to_string(),
            possible_required_control_entry_change_trust_levels: vec!["HIGH".to_string()],
            operation: "*".to_string(),
            default_consumer_permission: "YES".to_string(),
            possible_consumer_permissions: vec!["YES".to_string(), "NO".to_string()],
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MasterRegistrationControlEntry {
    #[serde(rename = "_typeName")]
    type_name: String,
    uid: String,
    domain: String,
    interface_name: String,
    default_required_trust_level: String,
    possible_required_trust_levels: Vec<String>,
    default_required_control_entry_change_trust_level: String,
    possible_required_control_entry_change_trust_levels: Vec<String>,
    default_provider_permission: String,
    possible_provider_permissions: Vec<String>,
}

impl MasterRegistrationControlEntry {
    pub fn new(
        in_uid: &String,
        in_domain: &String,
        in_interface_name: &String,
    ) -> MasterRegistrationControlEntry {
        MasterRegistrationControlEntry {
            type_name: "joynr.infrastructure.DacTypes.MasterRegistrationControlEntry".to_string(),
            uid: in_uid.to_string(),
            domain: in_domain.to_string(),
            interface_name: in_interface_name.to_string(),
            default_required_trust_level: "HIGH".to_string(),
            possible_required_trust_levels: vec!["HIGH".to_string()],
            default_required_control_entry_change_trust_level: "HIGH".to_string(),
            possible_required_control_entry_change_trust_levels: vec!["HIGH".to_string()],
            default_provider_permission: "YES".to_string(),
            possible_provider_permissions: vec!["YES".to_string(), "NO".to_string()],
        }
    }
}

#[derive(Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AccessStore {
    pub master_access_table: Vec<MasterAccessControlEntry>,
    pub master_registration_table: Vec<MasterRegistrationControlEntry>,
    mediator_access_table: Vec<String>,
    mediator_registration_table: Vec<String>,
    owner_access_table: Vec<String>,
    owner_registration_table: Vec<String>,
    domain_role_table: Vec<String>,
}

impl AccessStore {
    pub fn new(
        aces: Vec<MasterAccessControlEntry>,
        rces: Vec<MasterRegistrationControlEntry>,
    ) -> AccessStore {
        AccessStore {
            master_access_table: aces,
            master_registration_table: rces,
            mediator_access_table: Vec::new(),
            mediator_registration_table: Vec::new(),
            owner_access_table: Vec::new(),
            owner_registration_table: Vec::new(),
            domain_role_table: Vec::new(),
        }
    }
}
