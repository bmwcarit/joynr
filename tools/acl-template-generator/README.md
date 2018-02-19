## What is this good for?
The `acl-template-generator` is used to generate testing data for performance measurements of
the cluster-controller with ACL on. It accepts in input a list of domains, interfaces and userids
for which ACLs and RCLs should be generated. The output can contain ACL entries with or
without wildcards and is collected in a json file loadable by the cluster-controller.

## How do I use it?
```bash
USAGE:
    acl-template-generator [FLAGS] [OPTIONS]

FLAGS:
    -h, --help                   Prints help information
    -m, --include_exact_match    Include exact domain/interfaceName pair?
    -e, --random-entries         Create random entries with subdomains and subinterfaces with wildcards?
    -w, --use-wildcard           Should wildcard be used?
    -V, --version                Prints version information

OPTIONS:
    -d, --domains <domains>...                     Space separated list of domains.
    -i, --interfaces <interface_names>...          Space separated list of interface names.
    -o, --output_file_path <output_file_path>      Output path of the JSON file. [default: /tmp/access-store.json]
    -t, --random-iterations <random_iterations>    Number of random strings to generate. [default: 10]
    -u, --users <users>...                         Space separated list of users.
```

## Build instructions
```bash
# Prerequisite install Rust: https://www.rustup.rs/
# Then in this folder do:
cargo build
cargo run -- --help # this will show the help
```
