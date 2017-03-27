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
	var url = "http://" + document.location.host + document.location.pathname.replace(".html", "/");

function init() {
    aaData = [];

    
    var aoColumns = [
    {
        "sTitle": "Channel"
    },

    {
        "sTitle": "Active Connections"
    }
]
        
    $('#channels').dataTable({
        "bJQueryUI": true,
        //            "aaData": aaData,
        "aoColumns": aoColumns,
        "bAutoWidth": true,
        //"sPaginationType": "full_numbers"
        "bPaginate": false
    });
    
    update(); 
}

    
function update() {

    $.getJSON(url, function(data) {
        var items = [];
        
        for (var i=0; i<data.length; i++ ) {
            var entry = data[i];
            var channel = entry.name;
            var connections = entry.resources;
           
            
            var row = [];
            row.push(channel);
            row.push(connections);

            items.push(row);

        }
        
        
        var aaData = $('#channels').dataTable().fnGetData(); 
        var dataTable = $('#channels').dataTable();        
        dataTable.fnClearTable();
        dataTable.fnAddData(items); 

        setTimeout("update()", 5000);
    });
}

function addIds(items) {
    for (var i=0; i<items.length; i++) {
        items[i].push(items[i].join(''))
    }
}

function difference (referenceArray, differentArray) {
    
    var flatDifferentArray = [];
    var flatReferenceArray = [];
    for (var i=0; i<differentArray.length; i++) {
        var entry = differentArray[i].join(',');
        flatDifferentArray.push(entry);
    }
    
    for (var i=0; i<referenceArray.length; i++) {
        var entry = referenceArray[i].join(',');
        flatReferenceArray.push(entry);
    }
    
    var newItemsArray = _.difference(flatReferenceArray, flatDifferentArray);
    
    var returnArray = [];
    for (var i=0; i<newItemsArray.length; i++) {
        var entry = newItemsArray[i].split(',');
        returnArray.push(entry);
    }
    
    return returnArray;
}

