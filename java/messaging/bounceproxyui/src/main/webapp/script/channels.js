/*
 * #%L
 * joynr::java::backend-services::capabilities directory user interface
 * %%
 * Copyright (C) 2011 - 2012 BMW Car IT GmbH
 * %%
 * __________________
 * 
 * NOTICE:  Dissemination of this information or reproduction of this material 
 * is strictly  forbidden unless prior written permission is obtained from 
 * BMW Car IT GmbH.
 * #L%
 */
	var url = "http://" + document.location.host + "/bounceproxy/channels/";

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

