/* 1. GLOBALES*/

/* 2. FUNCIONES DE INICIALIZACION.*/
$(document).ready(function(){
	$("#execute_simulation").on("click", function(){
		send_uart();
	});
});

/* 3.0. FUNCIONES EN EL MENU DE SIMULACION*/

/*
 * 3.0.1. Manda la opcion seleccionada a la STM32
 */
function send_uart(){
	const selectedDisease = parseInt($('#selectedDis').val());
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
