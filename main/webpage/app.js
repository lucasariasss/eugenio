/* 1. GLOBALES*/
var wifiConnectInterval = null;
var configuracion_boton = 0

/* 2. FUNCIONES DE INICIALIZACION.*/
$(document).ready(function(){
	getSSID();
	getConnectInfo();
	$("#connect_wifi").on("click", function(){
		checkCredentials();
	});
	$("#disconnect_wifi").on("click", function(){
		disconnectWifi();
	});
	$("#execute_simulation").on("click", function(){
		send_uart();
	});
});

/* 3.0. FUNCIONES EN EL HEADER*/

/*
 * 3.0.1. Swicthea entre menu de configuracion es y menu de simulacion
 */
function switchConfig()
{	
	configuracion_boton += 1;
	if (configuracion_boton%2)
	{
		document.getElementById('home').style.display = 'none';
		document.getElementById('settings').style.display = 'block';
	}
	else
	{
		document.getElementById('home').style.display = 'block';
		document.getElementById('settings').style.display = 'none';
	}
}

/* 4.0. FUNCIONES EN EL MENU DE SIMULACION*/

/*
 * 4.0.1. Manda la opcion seleccionada a la STM32
 */
function send_uart(){
	const selectedDisease = parseInt($('#selectedDis').val()) + 1;
	const data = JSON.stringify({ disease: selectedDisease, timestamp: Date.now() });

	$.ajax({
		url: '/UARTmsg.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		contentType: 'application/json',
		data: data
	});
}

/* 5.0. FUNCIONES EN EL MENU DE CONECCION*/

/*
 * 5.0.1. Gets the ESP32's access point SSID for displaying on the web page.
 */
function getSSID()
{
	$.getJSON('/apSSID.json', function(data) {
		$("#ap_ssid").text(data["ssid"]);
	});
}

/* 5.1. CONECCION*/

/*
 * 5.1.1. Checkea que las credenciales esten bien
 */
function checkCredentials()
{
	errorList = "";
	credsOk = true;
	
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	if (selectedSSID == "")
	{
		errorList += "<h4 class='rd'>SSID obligatorio!</h4>";
		credsOk = false;
	}
	if (credsOk == false)
	{
		$("#wifi_connect_credentials_errors").html(errorList);
	}
	else
	{
		$("#wifi_connect_credentials_errors").html("");
		connectWifi();
	}
}

/*
 * 5.1.2. Connect Wifi function called using the SSID and password entered into the text fields
 */
function connectWifi()
{
	// Get the SSID and password
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	$.ajax({
		url: '/wifiConnect.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		data: {'timestamp': Date.now()}
	});
	
	startWifiConnectStatusInterval();
}

/*
 * 5.1.3. Starts the interval for checking the connection status
 */
function startWifiConnectStatusInterval()
{
	wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/*
 * 5.1.4. Gets wifi connection status
 */
function getWifiConnectStatus()
{
	var xhr = new XMLHttpRequest();
	var requestURL = "/wifiConnectStatus";
	xhr.open('POST', requestURL, false);
	xhr.send('wifi_connect_status');
	
	if (xhr.readyState == 4 && xhr.status == 200)
	{
		var response = JSON.parse(xhr.responseText);
		
		document.getElementById("wifi_connect_status").innerHTML = "Connecting...";
		
		if (response.wifi_connect_status == 2)
		{
			document.getElementById("wifi_connect_status").innerHTML = "<h4 class='rd'>Failed to connect.</h4>";
			stopWifiConnectStatusInterval();
		}
		else if (response.wifi_connect_status == 3)
		{
			document.getElementById("wifi_connect_status").innerHTML = "<h4 class='gr'>Connection success.</h4>";
			stopWifiConnectStatusInterval();
			getConnectInfo();
		}
	}
}

/*
 * 5.1.5. Clears the connection interval
 */
function stopWifiConnectStatusInterval()
{
	if (wifiConnectInterval != null)
	{
		clearInterval(wifiConnectInterval);
		wifiConnectInterval = null;
	}
}

/*
 * 5.1.6. Gets the connection information for displaying on the web page
 */
function getConnectInfo()
{
	$.getJSON('/wifiConnectInfo.json', function(data)
	{
		$("#connected_ap_label").html("Conectado a: ");
		$("#connected_ap").text(data.ap);
		
		$("#ip_address_label").html("Direcion IP: ");
		$("#wifi_connect_ip").text(data["ip"]);
		
		$("#netmask_label").html("Netmask: ");
		$("#wifi_connect_netmask").text(data["netmask"]);
		
		$("#gateway_label").html("Gateway: ");
		$("#wifi_connect_gw").text(data["gw"]);

		document.getElementById('disconnect_wifi').style.display = 'block';
	});
}

/*
 * 5.1.7. Checkbox de la contraseña
 */
function showPassword()
{
	var x = document.getElementById("connect_pass");
	if (x.type == "password")
	{
		x.type = "text";
	}
	else
	{
		x.type = "password";
	}
}

/* 5.2. DESCONECCION*/
/*
 * 5.2.1. Desconecta el wifi una vez que se aprieta el boton
 */
function disconnectWifi()
{
	$.ajax({
		url: '/wifiDisconnect.json',
		dataType: 'json',
		method: 'DELETE',
		cache: false,
		data: {'timestamp': Date.now()}
	});
	// actualiza la web
	setTimeout("location.reload(true);", 2000); 
}
