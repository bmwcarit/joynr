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
/*globals  */

/**
*   MockUp for DataService. This service is used if the query parameter 'simulation' is in the url.
*   It returns mocked lists and keeps changes local.
*   Used to test JS frontend without rest api instance.
*/
angular.module("acl-editor").service("DataServiceSimulation", ["$log", "$q", "$http", "$location", function ($log, $q, $http, $location) {

    $http.defaults.headers.common.Authorization = 'testApiKey';

    var restUrl = "/accesscontrol";
    var simulationDrtStorage = [{"uid":"owner1","domains":["domain1","domain2","domain3"],"role":"MASTER"},{"uid":"owner2","domains":["domain1","domain2","domain3"],"role":"OWNER"}];
    var simulationMasterACLStorage = [{uid: "id1", domain: "testDomain", interfaceName: "testInterface", operation: "operation1",
        defaultPermission: "ASK", possiblePermissions: "ASK, NO", defaultRequiredTrustLevel: "MID", possibleRequiredTrustLevels: "MID, HIGH", defaultRequiredAceChangeTrustLevel: "HIGH", possibleRequiredAceChangeTrustLevels: "HIGH, MID, LOW"}];
    var simulationOwnerACLStorage = [{uid: "id2", domain: "testDomain", interfaceName: "testInterface", operation: "operation2",
        permission: "ASK", requiredTrustLevel: "MID", requiredAceChangeTrustLevel: "MID"
    }];

/* DRT */
    this.readDrtEntries = function(){
        var params;
        return $q(function(resolve, reject) {
            resolve(angular.copy(simulationDrtStorage));
        });
    };

    this.createDrtEntry = function(data, id) {
        simulationDrtStorage[id] = data;
        return $q(function(resolve, reject) {
            resolve();
        });
    };

    this.updateDrtEntry = function(data, id) {
        simulationDrtStorage[id] = data;
        return $q(function(resolve, reject) {
            resolve();
        });
    };
    this.deleteDrtEntry = function(data) {
        var index = simulationDrtStorage.indexOf(data);
        simulationDrtStorage.splice(index, 1);
        return $q(function(resolve, reject) {
            resolve();
        });
    };

    /* Master ACL */

 this.readMasterAclEntries = function(){
        var params;
        return $q(function(resolve, reject) {
            resolve(angular.copy(simulationMasterACLStorage));
        });
    };

    this.createMasterAclEntry = function(data, id) {
        simulationMasterACLStorage[id] = data;
        return $q(function(resolve, reject) {
            resolve();
        });
    };

    this.updateMasterAclEntry = function(data, id) {
        simulationMasterACLStorage[id] = data;
        return $q(function(resolve, reject) {
            resolve();
        });
    };
    this.deleteMasterAclEntry = function(data) {
        var index = simulationMasterACLStorage.indexOf(data);
        simulationMasterACLStorage.splice(index, 1);
        return $q(function(resolve, reject) {
            resolve();
        });
    };


       /* Owner ACL */

     this.readOwnerAclEntries = function(){
            var params;
            return $q(function(resolve, reject) {
                resolve(angular.copy(simulationOwnerACLStorage));
            });
        };

        this.createOwnerAclEntry = function(data, id) {
            simulationOwnerACLStorage[id] = data;
            return $q(function(resolve, reject) {
                resolve();
            });
        };

        this.updateOwnerAclEntry = function(data, id) {
            simulationOwnerACLStorage[id] = data;
            return $q(function(resolve, reject) {
                resolve();
            });
        };
        this.deleteOwnerAclEntry = function(data) {
            var index = simulationOwnerACLStorage.indexOf(data);
            simulationOwnerACLStorage.splice(index, 1);
            return $q(function(resolve, reject) {
                resolve();
            });
        };

}]);
