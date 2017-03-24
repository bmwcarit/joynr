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
* Abstraction between http calls to rest service and angular-js app.
* Offers CRUD methods for domain role table, owner ACL and master ACL.
* Uses angular module $http to execute http methods and $q to create deferred results.
*/
angular.module("acl-editor").service("DataService", ["$log", "$q", "$http", function ($log, $q, $http) {


    var restUrl = "accesscontrol";

    this.readDrtEntries = function(){
        var params;
        return $q(function(resolve, reject) {
            $http.get(restUrl + "/domainroles",
                {params: params}).success(function(response) {
                        $log.info("Received " + response.length + " DRT entries");
                        resolve(response);
                    }).error(function(data, status, headers, config) {
                        reject("Failed to query DRT entries");
                    });
        });
    };

    this.createDrtEntry = function(data, id) {
        var params;
        return $q(function(resolve, reject) {
            $http.post(restUrl + "/domainroles/", data,
                {params: params}).success(function(response) {
                        $log.info("Created DRT entry");
                        resolve(response);
                    }).error(function(data, status, headers, config) {
                        reject("Failed to create DRT entry");
                    });
        });
    };

    this.updateDrtEntry = function(data, id) {
        return this.createDrtEntry(data, id);
    };

    this.deleteDrtEntry = function(data) {
        return $q(function(resolve, reject) {
            var uid = encodeURIComponent(data.uid)
            var role = encodeURIComponent(data.role)
            $http['delete'](restUrl + "/domainroles/" + uid + "/" + role
                    ).success(function(response) {
                        $log.info("Deleted DRT entry");
                        resolve(response);
                    }).error(function(data, status, headers, config) {
                        reject("Failed to delete DRT entry");
                    });
        });
    };


    /* Master ACL */

 this.readMasterAclEntries = function(){
         var params;
         return $q(function(resolve, reject) {
             $http.get(restUrl + "/masteracl",
                 {params: params}).success(function(response) {
                         $log.info("Received " + response.length + " master acl entries");
                         resolve(response);
                     }).error(function(data, status, headers, config) {
                         reject("Failed to query master acl entries");
                     });
         });
     };

     this.createMasterAclEntry = function(data, id) {
         var params;
         return $q(function(resolve, reject) {
             $http.post(restUrl + "/masteracl/", data,
                 {params: params}).success(function(response) {
                         $log.info("Created master ACL entry");
                         resolve(response);
                     }).error(function(data, status, headers, config) {
                         reject("Failed to create master ACL entry");
                     });
         });
     };

     this.updateMasterAclEntry = function(data, id) {
         return this.createMasterAclEntry(data, id);
     };

     this.deleteMasterAclEntry = function(data) {
         return $q(function(resolve, reject) {
             var uid = encodeURIComponent(data.uid)
             var domain = encodeURIComponent(data.domain)
             var interfaceName = encodeURIComponent(data.interfaceName)
             var operation = encodeURIComponent(data.operation)
             $http['delete'](restUrl + "/masteracl/" + uid + "/" + domain + "/" + interfaceName + "/" + operation
                 ).success(function(response) {
                         $log.info("Deleted master acl entry");
                         resolve(response);
                     }).error(function(data, status, headers, config) {
                         reject("Failed to delete master acl entry");
                     });
         });
     };

       /* Owner ACL */

 this.readOwnerAclEntries = function(){
         var params;
         return $q(function(resolve, reject) {
             $http.get(restUrl + "/owneracl",
                 {params: params}).success(function(response) {
                         $log.info("Received " + response.length + " owner acl entries");
                         resolve(response);
                     }).error(function(data, status, headers, config) {
                         reject("Failed to query owner acl entries");
                     });
         });
     };

     this.createOwnerAclEntry = function(data, id) {
         var params;
         return $q(function(resolve, reject) {
             $http.post(restUrl + "/owneracl/", data,
                 {params: params}).success(function(response) {
                         $log.info("Created owner ACL entry");
                         resolve(response);
                     }).error(function(data, status, headers, config) {
                         reject("Failed to create owner ACL entry");
                     });
         });
     };

     this.updateOwnerAclEntry = function(data, id) {
         return this.createOwnerAclEntry(data,id);
     };

     this.deleteOwnerAclEntry = function(data) {
         return $q(function(resolve, reject) {
             var uid = encodeURIComponent(data.uid)
             var domain = encodeURIComponent(data.domain)
             var interfaceName = encodeURIComponent(data.interfaceName)
             var operation = encodeURIComponent(data.operation)
             $http['delete'](restUrl + "/owneracl/" + uid + "/" + domain + "/" + interfaceName + "/" + operation
                    ).success(function(response) {
                         $log.info("Deleted owner acl entry");
                         resolve(response);
                     }).error(function(data, status, headers, config) {
                         reject("Failed to delete owner acl entry");
                     });
         });
     };


}]);
