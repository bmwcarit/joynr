# Cluster controller settings

## Access control
Access control verifies access permissions of requests sent from a consumer to a provider. It also controls if the application
can register a provider under a given domain/interface pair.
Replies, publications, subscription replies and multicasts are not being checked. Informations on which consumer can access which provider
are received from the global domain access controller (GDAC).

The following settings control access control behaviour at runtime:
* **access-control/enable**: if set to `true` permissions will be checked. No permission check will be performed
otherwise.
* **access-control/audit**: if set to `true` permissions will be checked but in case of failure, messages will not get dropped.
If set to `false`, no permission check will be performed. It does not have any effect if **access-control/enable** is set to `false`.
* **access-control/use-ldas-only**: if set to `true` no communication with the GDAC (global domain access controller) will be performed.
Otherwise the following two properties should be set:
  * **access-control/global-domain-access-controller-address**: serialized (JSON) address of a MqttAddress pointing to
the GDAC.
  * **access-control/global-domain-access-controller-participantid**: participantid of the GDAC.

For a concrete example, have a look at the following settings file:
[CCSettingsWithAccessControlEnabled.settings](../cpp/tests/resources/CCSettingsWithAccessControlEnabled.settings).

Additionally, it is also possible to provision access control entries to the cluster-controller. You can try by copying the
[CCAccessControlTest.entries](../cpp/tests/resources/CCAccessControlTest.entries) to the working
directory of the cluster-controller and rename it to `CCAccessControl.entries`.
