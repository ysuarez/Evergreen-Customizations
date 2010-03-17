dojo.require('openils.Util');
dojo.require('openils.User');
dojo.require('openils.widget.AutoGrid');
dojo.require('fieldmapper.OrgUtils');
dojo.require('openils.widget.OrgUnitFilteringSelect');
dojo.require('openils.widget.EditPane');
dojo.require("dijit.layout.StackContainer");
dojo.require('openils.PermaCrud');

var contextOrg;

function setup() {

    if(reqId) {
        drawRequest();
    } else {
        drawList();
        rGrid.cancelSelected = function() { doSelected('open-ils.acq.user_request.cancel.batch') };
        rGrid.setNoHoldSelected = function() { doSelected('open-ils.acq.user_request.set_no_hold.batch') };
    }
}

function drawRequest() {
    var pcrud = new openils.PermaCrud({ authtoken : openils.User.authtoken });
    var aur_obj = pcrud.retrieve('aur',reqId);

    // hide the grid and the context selector
    dijit.byId('stackContainer').forward();

    // draw a detail page for a particular request
    var div = document.getElementById('detail_content_pane');
    while (div.lastChild) { div.removeChild( div.lastChild ); }
    var pane = new openils.widget.EditPane({ 
        fmObject : aur_obj,
        readOnly : true
    });
    pane.domNode = div;
    pane.hideActionButtons = true;
    pane.startup();

    // including ability to add request to a picklist
    // and to "reject" it (aka apply a cancel reason)
}

function addToPicklist() {
    // reqId
    alert('stub');
}

function setNoHold() {
    // reqId
    alert('stub');
}

function cancelRequest() {
    // reqId
    alert('stub');
}

// format the title data as id:title
function getTitle(idx, item) {
    if(item) {
        return this.grid.store.getValue(item, 'id') + ':' + 
        this.grid.store.getValue(item, 'title');
    }
    return '';
}

// turn id:title into a url
function formatTitle(value) {
    if(value) {
        var parts = value.split(/:/);
        return '<a href="' + oilsBasePath + 
            '/acq/picklist/user_request/' + parts[0] + '">' + parts[1] + '</a>';
    }
}

function drawList() {
    buildGrid();

    var connect = function() {
        dojo.connect(contextOrgSelector, 'onChange',
            function() {
                contextOrg = this.attr('value');
                rGrid.resetStore();
                buildGrid();
            }
        );
    };

    new openils.User().buildPermOrgSelector(
        'CREATE_PICKLIST', contextOrgSelector, null, connect);
}

function buildGrid() {

    if(contextOrg == null)
        contextOrg = openils.User.user.ws_ou();

    rGrid.loadAll(
        {   order_by : {aur : 'request_date'},
            join : 'au' 
        },
        {
            cancel_reason : null,
            '+au' : {
                home_ou : fieldmapper.aou.descendantNodeList(contextOrg).map(
                    function(item) { return item.id(); })
            }
        }
    );
}

function doSelected(method) {
    try {
        var ids = [];
        dojo.forEach(
            rGrid.getSelectedItems(),
            function(item) {
                ids.push( rGrid.store.getValue(item,'id') );
            }
        );
        fieldmapper.standardRequest(
            [ 'open-ils.acq', method ],
            {   async: true,
                params: [openils.User.authtoken, ids],
                onresponse: function(r) {
                    try {
                        var result = openils.Util.readResponse(r);
                        if (typeof result.ilsevent != 'undefined') { throw(result); }
                    } catch(E) {
                        //dump('Error in acq/events.js, doSelected(), onresponse(): ' + E);
                        throw(E);
                    }
                },
                onerror: function(r) {
                    try {
                        var result = openils.Util.readResponse(r);
                        throw(result);
                    } catch(E) {
                        //dump('Error in acq/events.js, doSelected(), onerror(): ' + E);
                        throw(E);
                    }
                },
                oncomplete: function(r) {
                    try {
                        var result = openils.Util.readResponse(r);
                        rGrid.resetStore();
                        buildGrid();
                    } catch(E) {
                        //dump('Error in acq/events.js, doSelected(), oncomplete(): ' + E);
                        throw(E);
                    }
                }
            }
        );
    } catch(E) {
        //dump('Error in acq/events.js, doSelected(): ' + E);
        throw(E);
    }
}

openils.Util.addOnLoad(setup);

