# Cluster controller settings

## Access control
Access control verifies permissions of all requests sent from a consumer to a provider. Replies, publications,
subscription replies and multicasts are not being checked. Informations on which consumer can access which provider
are received from the global domain access controller (GDAC).

Access control must be activated at cmake-time with `JOYNR_ENABLE_ACCESS_CONTROL`. Upon activation via cmake,
the following settings are required:
* **access-control/enable**: if set to `true` permissions will be checked. No permission check will be performed
otherwise.
* **access-control/global-domain-access-controller-address**: serialized (JSON) address of a MqttAddress pointing to
the GDAC.
* **access-control/global-domain-access-controller-participantid**: participantid of the GDAC.

The GDAC will be used to keep the access permissions up to date.

For a concrete example, have a look at the following settings file:
[MessagingWithAccessControlEnabled.settings](../cpp/tests/resources/MessagingWithAccessControlEnabled.settings).

Additionally, it is also possible to provision access control entries to the cluster-controller. You can try by copying the
[CCAccessControlTest.entries](../cpp/tests/resources/CCAccessControlTest.entries) to the working
directory of the cluster-controller and rename it to `CCAccessControl.entries`.
