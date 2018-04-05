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
extern crate rand;

#[macro_use]
extern crate structopt;

#[macro_use]
extern crate serde_derive;

extern crate serde_json;

mod acl;

use acl::{AccessStore, MasterAccessControlEntry, MasterRegistrationControlEntry};
use rand::distributions::{IndependentSample, Range};
use serde_json::Error;
use structopt::StructOpt;

use std::io::prelude::*;
use std::fs::File;
use std::path::Path;

// Input parameters
#[derive(StructOpt, Debug)]
struct Opt {
    // Flags
    #[structopt(short = "w", long = "use-wildcard", help = "Should wildcard be used?")]
    use_wildcards: bool,

    #[structopt(short = "e", long = "random-entries",
                help = "Create random entries with subdomains and subinterfaces with wildcards?")]
    random_entries: bool,

    #[structopt(short = "m", long = "include_exact_match",
                help = "Include exact domain/interfaceName pair?")]
    include_exact_match: bool,

    // Parameters
    #[structopt(short = "t", long = "random-iterations",
                help = "Number of random strings to generate.", default_value = "10")]
    random_iterations: usize,

    #[structopt(short = "u", long = "users", help = "Space separated list of users.")]
    users: Vec<String>,

    #[structopt(short = "d", long = "domains", help = "Space separated list of domains.")]
    domains: Vec<String>,

    #[structopt(short = "i", long = "interfaces",
                help = "Space separated list of interface names.")]
    interface_names: Vec<String>,

    #[structopt(short = "o", long = "output_file_path",
                help = "Output path of the JSON file.", default_value = "/tmp/access-store.json")]
    output_file_path: String,
}

fn serialize_to_json(input: &AccessStore) -> Result<String, Error> {
    let serialized_input = serde_json::to_string(&input)?;
    Ok(serialized_input)
}

fn create_string(add_wildcard: bool, base_string: &String, sub_string_idx: usize) -> String {
    if add_wildcard {
        let mut output_string = base_string[..sub_string_idx].to_owned();
        output_string.push('*');
        return output_string;
    }
    base_string.clone()
}

fn main() {
    let opt = Opt::from_args();
    // println!("{:?}", opt);

    let mut domains: Vec<String> = Vec::new();
    let mut interface_names: Vec<String> = Vec::new();

    // if wildcards should be used, add randomly generated entries to domains and interface_names
    if opt.use_wildcards {
        domains.push(String::from("*"));
        interface_names.push(String::from("*"));

        if opt.random_entries {
            let mut rng = rand::thread_rng();
            for _ in 0..opt.random_iterations {
                for domain in &opt.domains {
                    let between = Range::new(0, domain.len() - 1);
                    domains.push(create_string(
                        rand::random(),
                        &domain,
                        between.ind_sample(&mut rng),
                    ));
                }
                for interface_name in &opt.interface_names {
                    let between = Range::new(0, interface_name.len() - 1);
                    interface_names.push(create_string(
                        rand::random(),
                        interface_name,
                        between.ind_sample(&mut rng),
                    ));
                }
            }
        }
    }

    if opt.include_exact_match {
        for domain in &opt.domains {
            domains.push(domain.clone());
        }
        for interface_name in &opt.interface_names {
            interface_names.push(interface_name.clone());
        }
    }

    // generate ACL & RCL
    let mut list_of_acl: Vec<MasterAccessControlEntry> = Vec::new();
    let mut list_of_rcl: Vec<MasterRegistrationControlEntry> = Vec::new();
    for uid in &opt.users {
        for domain in &domains {
            for int_name in &interface_names {
                let acl = MasterAccessControlEntry::new(&uid, &domain, &int_name);
                // println!("{:?}", &acl);
                list_of_acl.push(acl);

                let rcl = MasterRegistrationControlEntry::new(&uid, &domain, &int_name);
                // println!("{:?}", &rcl);
                list_of_rcl.push(rcl);
            }
        }
    }

    // generate Access Store
    let access_store: AccessStore = AccessStore::new(list_of_acl, list_of_rcl);

    let serialized_rcls = serialize_to_json(&access_store);
    match serialized_rcls {
        Ok(value) => {
            // println!("{}", value);
            write_to_file(value, opt.output_file_path);
        }
        Err(e) => println!("Serialization error: {:?}.", e),
    }

    println!(
        "File contains: {} ACLs and {} RCLs",
        &access_store.master_access_table.len(),
        &access_store.master_registration_table.len()
    );
}

fn write_to_file(json_string: String, output_file: String) {
    let path = Path::new(&output_file);
    let display = path.display();
    // Open a file in write-only mode, returns `io::Result<File>`
    let mut file = match File::create(&path) {
        Err(_) => panic!("Couldn't create {}", display),
        Ok(file) => file,
    };

    match file.write_all(json_string.as_bytes()) {
        Err(_) => panic!("Couldn't write to {}", display),
        Ok(_) => println!("Successfully wrote to {}", display),
    }
}
