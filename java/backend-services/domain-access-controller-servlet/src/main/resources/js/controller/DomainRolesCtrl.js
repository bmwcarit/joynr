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
angular.module("acl-editor").controller("DomainRolesCtrl", [ "$scope", "$location", "DataService", "DataServiceSimulation","FiltersPerColumnService", function($scope, $location, dataService_real, dataService_simulation, filtersPerColumnService){

    var dataService = $location.search().simulation ? dataService_simulation : dataService_real;

    $scope.domainRoles = [];
    filtersPerColumnService.init();

    var refresh = function(){
        dataService.readDrtEntries().then(function(result){
                $scope.domainRoles = result;
            });
    };
    refresh();


    $('#edit-modal').on('show.bs.modal', function (event) {
      var button = $(event.relatedTarget); // Button that triggered the modal
      var entry = button.data('entry'); // Extract info from data-* attributes
      var index = button.data('index');
      // If necessary, you could initiate an AJAX request here (and then do the updating in a callback).
      // Update the modal's content. We'll use jQuery here, but you could use a data binding library or other methods instead.
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
    $('#edit-update-btn').on('click', function(event){
        var index = $(this).data('index');

        dataService.updateDrtEntry($scope.editableEntry).then(
            function(){
                console.log('Successfully updated DRT entry');
                $scope.domainRoles[index] = $scope.editableEntry;
            },
            function(errorMsg){
                console.log('Failed to update DRT entry. Cause: ' + errorMsg);
            }
        );
        $('#edit-modal').modal('hide');
      });

      $('#edit-add-btn').on('click', function(event){
              dataService.createDrtEntry($scope.editableEntry).then(
                  function(){
                      console.log('Successfully added DRT entry');
                      $scope.domainRoles.push($scope.editableEntry);
                  },
                  function(errorMsg){
                      console.log('Failed to add DRT entry. Cause: ' + errorMsg);
                  }
              );
              $('#edit-modal').modal('hide');
            });

      $('#del-apply-btn').on('click', function(event){
        var index = $(this).data('index');

        dataService.deleteDrtEntry($scope.editableEntry).then(
            function(){
                console.log('Successfully deleted DRT entry');
                $scope.domainRoles.splice(index, 1);
            },
            function(errorMsg){
                console.log('Failed to delete DRT entry. Cause: ' + errorMsg);
            }
        );
        $('#delete-modal').modal('hide');
      });
}]);
