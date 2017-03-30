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
angular.module("acl-editor").controller("NavController", [ "$scope", "$location", function($scope, $location){

    $scope.activeRoute = 'masterAcl';

    $scope.loadMasterAcl = function () {
        $scope.activeRoute = '/master';
        $location.path('/master');
    };

      $scope.loadOwnerAcl = function () {
        $scope.activeRoute = '/owner';
        $location.path('/owner');
    };

      $scope.loadDomainRoles = function () {
        $scope.activeRoute = '/domainroles';
        $location.path('/domainroles');
    };

    $scope.isActive = function(viewLocation){
        return viewLocation === $location.path();
    };



}]);