/*
 * #%L
 * joynr::java::backend-services::capabilities directory user interface
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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


function init() {
    aaData = [];

    
    var aoColumns = [
    {
        "sTitle": "Channel"
    },

    {
        "sTitle": "Participant"
    },
    
    {
        "sTitle": "Domain"
    },

    {
        "sTitle": "Interface"
    },

    {
        "sTitle": "Priority"
    },
    
    {
        "sTitle": "Version"
    }
]
        
    $('#capabilities').dataTable({
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
    $.getJSON('capabilities.json', function(data) {
        var items = [];
        
        for (var i=0; i<data.length; i++ ) {
            var entry = data[i];
            var channel = entry.channelId;
            var participant = entry.participantId;
            var domain = entry.domain;
            var interfaceName = entry.interfaceName;
            var priority = entry.providerQos.priority;
            var version = entry.providerQos.version;
            
			if(channel === undefined) {
				channel = "undefined";
			}
			if(participant === undefined) {
				participant = "undefined";
			}
			if(domain === undefined) {
				domain = "undefined";
			}
			if(interfaceName === undefined) {
				interfaceName = "undefined";
			}
			if(priority === undefined) {
				priority = "undefined";
			}
			if(version === undefined) {
				version = "undefined";
			}
			
            var row = [];
            row.push(channel);
            row.push(participant);
            row.push(domain);
            row.push(interfaceName);
            row.push(priority);            
            row.push(version);
            items.push(row);

        }
        
        
        var aaData = $('#capabilities').dataTable().fnGetData(); 
        var dataTable = $('#capabilities').dataTable();        
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

