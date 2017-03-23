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
angular.module("acl-editor").controller("OwnerAclCtrl", [ "$scope", "$location", "DataService", "DataServiceSimulation","FiltersPerColumnService", function($scope, $location, dataService_real, dataService_simulation, filtersPerColumnService){


    var dataService = $location.search().simulation ? dataService_simulation : dataService_real;

    $scope.ownerACL = [];
    filtersPerColumnService.init();

    var refresh = function(){
        dataService.readOwnerAclEntries().then(function(result){
            $scope.ownerACL = result;
        });
    };
    refresh();


    $('#edit-modal').on('show.bs.modal', function (event) {
      var button = $(event.relatedTarget); // Button that triggered the modal
      var entry = button.data('entry'); // Extract info from data-* attributes
      var index = button.data('index');
      var modal = $(this);

      $scope.$apply(function(){
        $scope.editableEntry = entry;
        $scope.mode = button.data('mode');
      });
      modal.find('#edit-update-btn').data('index', index);

    });

    $('#delete-modal').on('show.bs.modal', function (event) {
      var button = $(event.relatedTarget); // Button that triggered the modal
      var entry = button.data('entry'); // Extract info from data-* attributes
      var index = button.data('index');

      $scope.$apply(function(){
              $scope.editableEntry = entry;
            });
      $(this).find('#del-apply-btn').data('index', index);
    });

    $('#edit-update').on('click', function(event){
        var index = $(this).data('index');

        dataService.updateOwnerAclEntry($scope.editableEntry).then(
            function(){
                console.log('Successfully updated Owner ACL entry');
                $scope.ownerACL[index] = $scope.editableEntry;
            },
            function(errorMsg){
                console.log('Failed to update Owner ACL entry. Cause: ' + errorMsg);
            }
        );
        $('#edit-modal').modal('hide');
      });

      $('#edit-add-btn').on('click', function(event){
              dataService.createOwnerAclEntry($scope.editableEntry).then(
                  function(){
                      console.log('Successfully added Owner ACL entry');
                      $scope.ownerACL.push($scope.editableEntry);
                  },
                  function(errorMsg){
                      console.log('Failed to add Owner ACL entry. Cause: ' + errorMsg);
                  }
              );
              $('#edit-modal').modal('hide');
            });

      $('#del-apply-btn').on('click', function(event){
        var index = $(this).data('index');

        dataService.deleteOwnerAclEntry($scope.editableEntry).then(
            function(){
                console.log('Successfully deleted Owner ACL entry');
                $scope.ownerACL.splice(index, 1);
            },
            function(errorMsg){
                console.log('Failed to delete Owner ACL entry. Cause: ' + errorMsg);
            }
        );
        $('#delete-modal').modal('hide');
      });
}]);