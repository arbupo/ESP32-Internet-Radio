/**
 * ----------------------------------------------------------------------------
 * ESP32 Cloc - Alarm Clock
 * ----------------------------------------------------------------------------
 * Author: Yves BOURDON
 * Date:   Sept-Nov 2022
 * ----------------------------------------------------------------------------
 */

// ----------------------------------------------------------------------------
// Global constants
// ----------------------------------------------------------------------------
// Periodic data update
const dataUpdate = 800; // 0.8 second (in milliseconds)

// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------

// Default asynchronous request manager (classical AJAX method)
var xhttp = new XMLHttpRequest();

/**
 * We will need to read some data or update some elements of the HTML page.
 * So we need to define variables to reference them more easily throughout
 * the program.
 */

// Input fields for Wifi parameters
var ssid;
var pwd;

// Input fields for Chanel parameters
var chanel;
var station;
var urlStation;

// Input fields for selection parameters
var selChan;
var selVol;

// ESP32 control buttons
var btnDefault;
var btnReboot;

// Updated time,volume,chanel,station and url display
// -----------------------------------------------
var heure;
var volume;
var cur_station;
var cur_chanel;
var cur_url;
var rssi;

// ----------------------------------------------------------------------------
// Initialization on full loading of the HTML page
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
	initData();
    initButtons();
    initWifiSetup();
    initChanelSetup();
    initSelSetup();
}


function initData() {
    heure = document.getElementById('heure');
    volume = document.getElementById('volume');
    cur_station = document.getElementById('cur_station');
    cur_chanel = document.getElementById('cur_chanel');
    cur_url = document.getElementById('cur_url');
    rssi = document.getElementById('rssi');
    // a timing event manager is initialized
    // which must be triggered every 0.8 second to refresh
    // the display of the current time chanel name stream ....
    updateData();
    setInterval(updateData, dataUpdate);
}

function updateData() {
    xhrRequest('/dataUpdate', (val) => { setData(val); });
}

function setData(val) {
  var valParts = val.split('|');
  heure.innerText = valParts[0];
  volume.innerText = valParts[1];
  cur_station.innerText = valParts[2];
  cur_chanel.innerText = valParts[3];
  cur_url.innerText = valParts[4];
  rssi.innerText = valParts[5];
}


// -----------------------------------------------------------------------------
// Initialization and control of the input fields for Wifi and Chanel parameters
// -----------------------------------------------------------------------------

function initSelSetup() {
    selChan = document.getElementById('selChan');
    selVol = document.getElementById('selVol');
}

function initChanelSetup() {
    chanel = document.getElementById('chanel');
    station = document.getElementById('station');
    urlStation = document.getElementById('urlStation');

}

function initWifiSetup() {
    ssid = document.getElementById('ssid');
    pwd = document.getElementById('pwd');
}


// While the user is entering a value
// ----------------------------------

function digitOnly(event) {
    return /[\d]/.test(event.key); // accept digit only
}

function anyChar(event) {
    return test(event.key); // accept all codes
}

function saveWifi(){
    xhrRequest(`/setwifi?ssid=${ssid.value}&pwd=${pwd.value}`);
}

function saveChanel(){
    xhrRequest(`/setchanel?chanel=${chanel.value}&station=${station.value}&urlStation=${urlStation.value}`);
	setTimeout(function() {location.reload()}, 200);
}

function saveSel(){
    xhrRequest(`/setsel?selChan=${selChan.value}&selVol=${selVol.value}`);
}


// ----------------------------------------------------------------------------
// Initialization and handling of the ESP32 control buttons
// ----------------------------------------------------------------------------

function initButtons() {
    btnDefault = document.getElementById('default');
    btnReboot  = document.getElementById('reboot');
    btnDefault.addEventListener('click', onDefault);
    btnReboot.addEventListener('click', onReboot);
}

// Factory reset event manager
// ---------------------------

function onDefault(event) {
    // asynchronous call of the remote routine with the classical method
    xhrRequest('/reset');
    ssid.value="-----------";
    pwd.value="-----------";
}

// Event manager for restarting ESP32
// ----------------------------------

function onReboot(event) {
    xhrRequest('/reboot');
    // sends reboot command to the ESP32
    //alert("Le système va redémarrer!");
    ssid.value="-----------";
    pwd.value="-----------";
}

// -------------------------------------------------------
// AJAX requests
// -------------------------------------------------------

// Using standard vanilla XHR (XMLHttpRequest method)
// @see https://www.w3schools.com/xml/ajax_xmlhttprequest_send.asp

function xhrRequest(path, callback) {

    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            // callback is optional!
            typeof callback === 'function' && callback(this.responseText);
        }
    };

    xhttp.open('GET', path, true);
    xhttp.send();
}

