/* 1. GLOBALES*/
var wifiConnectInterval = null;
var configuracion_boton = 0

const mapaDeEnfermedades = {
	"000": ["c1", "c2", "c3", "c4"],
	"001": ["c1", "c2", "c3", "c4"],
	"002": ["c4"],
	"003": ["c4"],
	"004": ["c1"],
	"005": ["c1"],
	"006": ["c1"],
	"007": ["c3"],
	"008": ["c3"],
	"009": ["c1"],
	"010": ["c1", "c2", "c3", "c4"],
	"011": ["c3"],
	"012": ["c3"],
	"013": ["c3", "c4"],
	"014": ["c3", "c4"],
	"015": ["c4"],
	"016": ["c4"],
	"017": ["c4"],
	"018": ["c4"],
	"019": ["c3", "c4"],
	"020": ["c1", "c2"],
	"021": ["c2"],
	"022": ["c2"],
	"023": ["c1", "c2"],
	"024": ["c4"]
};

/* 2. FUNCIONES DE INICIALIZACION.*/
$(document).ready(function(){
	// Solo pido info de WiFi si la página tiene la UI de WiFi
	const hasWifiUI = $("#WiFiConnect").length || $("#ConnectInfo").length;
	if (hasWifiUI) {
		getSSID();
		getConnectInfo();
	}

	const $btnConnect = $("#connect_wifi");
	if ($btnConnect.length) {
		$btnConnect.on("click", function(){
			checkCredentials();
		});
	}

	const $btnDisconnect = $("#disconnect_wifi");
	if ($btnDisconnect.length) {
		$btnDisconnect.on("click", function(){
			disconnectWifi();
		});
	}

	const $btnExecute = $("#execute_simulation");
	if ($btnExecute.length) {
		$btnExecute.on("click", function(){
			send_uart();
		});
	}
});


/* 3.0. FUNCIONES EN EL HEADER*/

/*
 * 3.0.1. Swicthea entre menu de configuraciones y menu de simulacion
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
	const selectedDisease = parseInt($('#selectedDis').val());
	const data = JSON.stringify({ enfermedad: selectedDisease, timestamp: Date.now() });

	$.ajax({
		url: '/UARTmsg.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		contentType: 'application/json',
		data: data
	});
}

/* 
 * 4.0.2. Muestra los circulos correspondientes a la enfermedad seleccionada
*/
function showCircles(){
	const selectedDisease = $('#selectedDis').val();
	expuestos = document.getElementsByClassName('circulo-activo');
	const mostrarCirculos = mapaDeEnfermedades[selectedDisease];
	while (expuestos.length > 0){
		expuestos[0].classList.remove('circulo-activo');
	}
	for (let i=0; i<mostrarCirculos.length; i++){
		document.getElementById(mostrarCirculos[i]).classList.add('circulo-activo');
	}
}


/* 5.0. FUNCIONES EN EL MENU DE CONEXION*/

/*
 * 5.0.1. Gets the ESP32's access point SSID for displaying on the web page.
 */
function getSSID()
{
	$.getJSON('/apSSID.json', function(data) {
		$("#ap_ssid").text(data["ssid"]);
	});
}

/* 5.1. CONEXION*/

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

		const disconnectBtn = document.getElementById('disconnect_wifi');
		if (disconnectBtn) {
			disconnectBtn.style.display = 'block';
		}
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

/* 5.2. DESCONEXION*/
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
