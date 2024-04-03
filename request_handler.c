#include "logger.h"
#include "fake_sensors.h"
#include "request_handler.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#define UPTIME(start) time(0) - start

#define PASSWORD			"12345678"
#define FAKE_SENSORS_FILE	"fake_sensors.txt"
#define AUTH_SUCCESS_COOKIE	"Authorized"
#define AUTH_FAILURE_COOKIE	"Unauthorized"
#define COOKIE_MAX_AGE		"300"


typedef enum request_t {
	NONE,
	AUTH_POST,
	AUTH_HOME,
	DEV_STATUS,
	DEV_HOME,
	PUMP_STATUS,
	PUMP_HOME,
    PUMP_ON,
    PUMP_OFF
} request_t;

request_t get_request_type(const char *request_buffer);

void set_header(char *response_buffer, request_t request_type);
void set_authorization(char *response_buffer, const char *request_buffer, request_t request_type);
void set_status_json(char *response_buffer, request_t request_type, time_t start);
void set_precontent(char *response_buffer, request_t request_type);
void set_content(char *response_buffer, request_t request_type, time_t start);
void set_footer(char *response_buffer, request_t request_type);
void set_scripts(char *response_buffer, request_t request_type);


void handle_request(const char *request_buffer, char *response_buffer, time_t start)
{
	// finding request type
	request_t request_type = get_request_type(request_buffer);
	// response header
	strcpy(response_buffer, "");
	set_header(response_buffer, request_type);
	set_authorization(response_buffer, request_buffer, request_type);
	// json status
	set_status_json(response_buffer, request_type, start);
	// precontent
	set_precontent(response_buffer, request_type);
	// content
	set_content(response_buffer, request_type, start);
	// footer
	set_footer(response_buffer, request_type);
	// scripts
	set_scripts(response_buffer, request_type);
	// logging
	switch (request_type) {
		case AUTH_POST:
			write_log(DEBUG, "Sent authorization cookie.");
			break;
		case DEV_STATUS:
		case PUMP_STATUS:
			write_log(DEBUG, "Sent JSON response.");
			break;
		case DEV_HOME:
		case AUTH_HOME:
		case PUMP_HOME:
			write_log(INFO, "Sent HTML page.");
			break;
		case PUMP_ON:
			write_log(INFO, "Turned pump ON from website.");
			break;
		case PUMP_OFF:
			write_log(INFO, "Turned pump OFF from website.");
			break;
		default:
			break;
	}
}


request_t get_request_type(const char *request_buffer)
{
	if (strstr(request_buffer, "POST /auth"))
		return AUTH_POST;
	else if (strstr(request_buffer, "GET /dev/status"))
		return DEV_STATUS;
	else if (strstr(request_buffer, "GET /pump/status"))
		return PUMP_STATUS;
	else if (!strstr(request_buffer, AUTH_SUCCESS_COOKIE))
		return AUTH_HOME; 
	else if (strstr(request_buffer, "GET /dev"))
		return DEV_HOME;
	else if (strstr(request_buffer, "GET /"))
		return PUMP_HOME;
	else if (strstr(request_buffer, "POST /pump") && strstr(request_buffer, "ON"))
		return PUMP_ON;
	else if (strstr(request_buffer, "POST /pump") && strstr(request_buffer, "OFF"))
		return PUMP_OFF;
	return NONE;
}

void set_header(char *response_buffer, request_t request_type)
{
	switch (request_type) {
		case DEV_STATUS:
			strcat(response_buffer, "HTTP/1.1 200 OK\r\n");
			strcat(response_buffer, "Content-type: application/json\r\n\r\n");
			break;
		case PUMP_STATUS:
			strcat(response_buffer, "HTTP/1.1 200 OK\r\n");
			strcat(response_buffer, "Content-type: application/json\r\n\r\n");
			break;
		case DEV_HOME:
		case PUMP_HOME:
			strcat(response_buffer, "HTTP/1.1 200 OK\r\n");
			strcat(response_buffer, "Content-type: text/html\r\n\r\n");
			break;
		case AUTH_HOME:
			strcat(response_buffer, "HTTP/1.1 203 Non-Authoritative Information\r\n");
			strcat(response_buffer, "Content-type: text/html\r\n\r\n");
			break;
		case PUMP_ON:
			set_sensor_value(RELAY, 1);
			strcat(response_buffer, "HTTP/1.1 200 OK\r\n");
			strcat(response_buffer, "Content-type: text/plain\r\n\r\nON\r\n");
			break;
		case PUMP_OFF:
			set_sensor_value(RELAY, 0);
			strcat(response_buffer, "HTTP/1.1 200 OK\r\n");
			strcat(response_buffer, "Content-type: text/plain\r\n\r\nOFF\r\n");
			break;
		default:
			break;
	}
}

void set_authorization(char *response_buffer, const char *request_buffer, request_t request_type)
{
	if (request_type != AUTH_POST) return;
	if (strstr(request_buffer, PASSWORD)) {
		strcat(response_buffer, "HTTP/1.1 200 OK\r\n");
		strcat(response_buffer, "Content-type: text/plain\r\n");
		strcat(response_buffer, "Set-cookie: " AUTH_SUCCESS_COOKIE "; Max-age=" COOKIE_MAX_AGE "; SameSite=None; Secure\r\n\r\n");
		strcat(response_buffer, "SUCCESS\r\n");
	} else {
		strcat(response_buffer, "HTTP/1.1 203 Non-Authoritative Information\r\n");
		strcat(response_buffer, "Content-type: text/plain\r\n");
		strcat(response_buffer, "Set-cookie: " AUTH_FAILURE_COOKIE "; Max-age=" COOKIE_MAX_AGE "; SameSite=None; Secure\r\n\r\n");
		strcat(response_buffer, "FAILURE\r\n");
	}
}

void set_status_json(char *response_buffer, request_t request_type, time_t start)
{
	char line[100];
	switch (request_type) {
		case PUMP_STATUS:
			strcat(response_buffer, "{\r\n");
			sprintf(line, "  \"t_top\": %d,\r\n", get_sensor_value(T_TOP));
			strcat(response_buffer, line);
			sprintf(line, "  \"t_bottom\": %d,\r\n", get_sensor_value(T_BOTTOM));
			strcat(response_buffer, line);
			sprintf(line, "  \"r_bottom\": %d,\r\n", get_sensor_value(R_BOTTOM));
			strcat(response_buffer, line);
			sprintf(line, "  \"relay\": %d\r\n", get_sensor_value(RELAY));
			strcat(response_buffer, line);
			strcat(response_buffer, "}\r\n");
			break;
		case DEV_STATUS:
			strcat(response_buffer, "{\r\n");
			sprintf(line, "  \"uptime\": %ld,\r\n", UPTIME(start));
			strcat(response_buffer, line);
			sprintf(line, "  \"free_heap\": %ld\r\n", 150+time(0)%100);
			strcat(response_buffer, line);
			strcat(response_buffer, "}\r\n");
			break;
		default:
			break;
	}
}

void set_precontent(char *response_buffer, request_t request_type)
{
	switch (request_type) {
		case DEV_HOME:
		case AUTH_HOME:
		case PUMP_HOME:
			strcat(response_buffer, "<!DOCTYPE html>\r\n");
			strcat(response_buffer, "<html lang='en' data-bs-theme='dark'>\r\n");
			strcat(response_buffer, "  <head>\r\n");
			strcat(response_buffer, "    <meta charset='UTF-8'>\r\n");
			strcat(response_buffer, "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n");
			strcat(response_buffer, "    <title>Smart pump</title>\r\n");
			strcat(response_buffer, "    <link rel='icon' type='image/x-icon' href='https://avatars.githubusercontent.com/t/6664638'>\r\n");
			strcat(response_buffer, "    <link rel='preconnect' href='https://fonts.googleapis.com'>\r\n");
			strcat(response_buffer, "    <link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>\r\n");
			strcat(response_buffer, "    <link href='https://fonts.googleapis.com/css2?family=Open+Sans:ital,wght@0,300..800;1,300..800&display=swap' rel='stylesheet'>\r\n");
			strcat(response_buffer, "    <link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css' rel='stylesheet' integrity='sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH' crossorigin='anonymous'>\r\n");
			strcat(response_buffer, "    <style>\r\n");
			strcat(response_buffer, "        * { font-family: 'Open Sans' 'Helvetica Neue' Helvetica sans-serif; }\r\n");
			strcat(response_buffer, "        nav.navbar { color: white; }\r\n");
			strcat(response_buffer, "        div#main-body { width: 100%; }\r\n");
			strcat(response_buffer, "    </style>\r\n");
			strcat(response_buffer, "    <script>\r\n");
			strcat(response_buffer, "      theme = localStorage.getItem('theme')\r\n");
			strcat(response_buffer, "      if (theme) document.documentElement.dataset.bsTheme = theme\r\n");
			strcat(response_buffer, "    </script>\r\n");
			strcat(response_buffer, "  </head>\r\n");
			strcat(response_buffer, "  <body class='d-flex flex-column min-vh-100'>\r\n");
			strcat(response_buffer, "    <nav class='navbar shadow bg-primary py-2 text-center'>\r\n");
			strcat(response_buffer, "      <div class='container-md'>\r\n");
			strcat(response_buffer, "        <div>\r\n");
			strcat(response_buffer, "          <img src='https://avatars.githubusercontent.com/t/6664638' alt='iot-club-icon' height='48px'>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='mt-2 pb-0'>\r\n");
			strcat(response_buffer, "          <h4><b>Automatic Pump</b></h4>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div>\r\n");
			strcat(response_buffer, "          <a class='btn btn-primary' onclick='toggle_theme()'>\r\n");
			strcat(response_buffer, "            <svg id='theme-toggle' xmlns='http://www.w3.org/2000/svg' width='16' height='16' fill='white' class='bi bi-circle-half' viewBox='0 0 16 16'>\r\n");
			strcat(response_buffer, "              <path d='M8 15A7 7 0 1 0 8 1zm0 1A8 8 0 1 1 8 0a8 8 0 0 1 0 16'></path>\r\n");
			strcat(response_buffer, "            </svg>\r\n");
			strcat(response_buffer, "          </a>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "      </div>\r\n");
			strcat(response_buffer, "    </nav>\r\n");
			break;
		default:
			break;
	}
}

void set_content(char *response_buffer, request_t request_type, time_t start)
{
	char line[200];
	switch (request_type) {
		case PUMP_HOME:
			strcat(response_buffer, "    <div id='main-body' class='container-md text-center'>\r\n");
            strcat(response_buffer, "      <div class='card shadow-sm p-4 mt-4'>\r\n");
            strcat(response_buffer, "        <b class='card-title'>Tank Status</b>\r\n");
            strcat(response_buffer, "        <div class='card-body'>\r\n");
            strcat(response_buffer, "          <div class='row'>\r\n");
            strcat(response_buffer, "            <div class='col'><b>Top</b></div>\r\n");
			sprintf(line, "            <div class='col' id='t_top'>%s</div>\r\n", get_sensor_value(T_TOP) ? "ON" : "OFF");
            strcat(response_buffer, line);
            strcat(response_buffer, "          </div>\r\n");
            strcat(response_buffer, "          <div class='row'>\r\n");
            strcat(response_buffer, "            <div class='col'><b>Bottom</b></div>\r\n");
			sprintf(line, "            <div class='col' id='t_bottom'>%s</div>\r\n", get_sensor_value(T_BOTTOM) ? "ON" : "OFF");
            strcat(response_buffer, line);
            strcat(response_buffer, "          </div>\r\n");
            strcat(response_buffer, "        </div>\r\n");
            strcat(response_buffer, "      </div>\r\n");
            strcat(response_buffer, "      <br>\r\n");
            strcat(response_buffer, "      <div class='card shadow-sm p-4'>\r\n");
            strcat(response_buffer, "        <b class='card-title'>Reservoir Status</b>\r\n");
            strcat(response_buffer, "        <div class='card-body'>\r\n");
            strcat(response_buffer, "          <div class='row'>\r\n");
            strcat(response_buffer, "            <div class='col'><b>Bottom</b></div>\r\n");
			sprintf(line, "            <div class='col' id='r_bottom'>%s</div>\r\n", get_sensor_value(R_BOTTOM) ? "ON" : "OFF");
			strcat(response_buffer, line);
            strcat(response_buffer, "          </div>\r\n");
            strcat(response_buffer, "        </div>\r\n");
            strcat(response_buffer, "      </div>\r\n");
            strcat(response_buffer, "      <br>\r\n");
            strcat(response_buffer, "      <div class='card shadow-sm p-4'>\r\n");
            strcat(response_buffer, "        <b class='card-title'>Pump Status</b>\r\n");
            strcat(response_buffer, "        <div class='card-body'>\r\n");
            strcat(response_buffer, "          <div class='row'>\r\n");
            strcat(response_buffer, "            <div class='col'><b>Bottom</b></div>\r\n");
			sprintf(line, "            <div class='col' id='relay'>%s</div>\r\n", get_sensor_value(RELAY) ? "ON" : "OFF");
			strcat(response_buffer, line);
            strcat(response_buffer, "          </div>\r\n");
            strcat(response_buffer, "        </div>\r\n");
            strcat(response_buffer, "      </div>\r\n");
            strcat(response_buffer, "      <br>\r\n");
            strcat(response_buffer, "      <div class='card shadow-sm p-4'>\r\n");
            strcat(response_buffer, "        <b class='card-title'>Pump Control</b>\r\n");
            strcat(response_buffer, "        <div class='card-body row'>\r\n");
			sprintf(line, "          <a class='btn shadow-sm btn-primary disabled' id='relay_control' onclick='send_pump_control()'>%s</a>\r\n", get_sensor_value(RELAY) ? "OFF" : "ON");
            strcat(response_buffer, line);
            strcat(response_buffer, "        </div>\r\n");
            strcat(response_buffer, "      </div>\r\n");
            strcat(response_buffer, "    </div>\r\n");
			break;
		case AUTH_HOME:
			strcat(response_buffer, "    <div id='main-body' class='container-md text-center'>\r\n");
			strcat(response_buffer, "      <div class='card m-4 p-4'>\r\n");
			strcat(response_buffer, "        <div class='m-4'>\r\n");
			strcat(response_buffer, "          <b class='card-title'>ESP-WROOM-32</b>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='m-4'>\r\n");
			strcat(response_buffer, "          <div class='row'>\r\n");
			strcat(response_buffer, "            <div class='col'>\r\n");
			strcat(response_buffer, "              <label for='password_input'><b>Password</b></label>\r\n");
			strcat(response_buffer, "            </div>\r\n");
			strcat(response_buffer, "            <div class='col'>\r\n");
			strcat(response_buffer, "              <input type='password' class='px-1 container-md' id='password_input' required autofocus>\r\n");
			strcat(response_buffer, "            </div>\r\n");
			strcat(response_buffer, "          </div>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "      </div>\r\n");
			strcat(response_buffer, "    </div>\r\n");
			break;
		case DEV_HOME:
			strcat(response_buffer, "    <div id='main-body' class='container-md text-center'>\r\n");
			strcat(response_buffer, "      <div class='p-4 mt-4'>\r\n");
			strcat(response_buffer, "        <b class='card-title'>ESP-WROOM-32</b>\r\n");
			strcat(response_buffer, "      </div>\r\n");
			strcat(response_buffer, "    </div>\r\n");
			strcat(response_buffer, "    <div class='container-md'>\r\n");
			strcat(response_buffer, "      <div class='p-4 mt-4'>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>Uptime</b></d>\r\n");
			sprintf(line, "          <d class='col'><span id='uptime'>%ld</span> s</d>\r\n", UPTIME(start));
			strcat(response_buffer, line);
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>Free heap</b></d>\r\n");
			sprintf(line, "          <d class='col'><span id='free_heap'>%ld</span> KB</d>\r\n", 150+time(0)%100);
			strcat(response_buffer, line);
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <hr>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>Cores</b></d>\r\n");
			strcat(response_buffer, "          <d class='col'><span id='num_cores'>2</span></d>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>Chip model</b></d>\r\n");
			strcat(response_buffer, "          <d class='col'><span id='chip_model'>ESP32-D0WD-V3</span></d>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>SDK version</b></d>\r\n");
			strcat(response_buffer, "          <d class='col'><span id='sdk_version'>v4.4.6-dirty</span></d>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>Connectivity</b></d>\r\n");
			strcat(response_buffer, "          <d class='col'><span id='connectivity'>WiFi-BT-BLE</span></d>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>CPU frequency</b></d>\r\n");
			strcat(response_buffer, "          <d class='col'><span id='cpu_frequency'>240</span> MHz</d>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <hr>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>Free sketch space</b></d>\r\n");
			strcat(response_buffer, "          <d class='col'><span id='free_sketch_space'>1280</span> KB</d>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "        <div class='row'>\r\n");
			strcat(response_buffer, "          <d class='col'><b>Sketch MD5 checksum</b></d>\r\n");
			strcat(response_buffer, "          <d class='col'><span id='sketch_md5'>1383a4e8f0d5ef99da89a48e1e121bdd</span></d>\r\n");
			strcat(response_buffer, "        </div>\r\n");
			strcat(response_buffer, "      </div>\r\n");
			strcat(response_buffer, "    </div>\r\n");
			break;
		default:
			break;
	}
}

void set_footer(char *response_buffer, request_t request_type)
{
	switch (request_type) {
		case DEV_HOME:
		case AUTH_HOME:
		case PUMP_HOME:
			strcat(response_buffer, "    <footer class='text-center pb-2 mt-auto'>\r\n");
			strcat(response_buffer, "      VBDCSS IoT Club @ 2024\r\n");
			strcat(response_buffer, "    </footer>\r\n");
			break;
		default:
			break;
	}
}

void set_scripts(char *response_buffer, request_t request_type)
{
	switch (request_type) {
		case PUMP_HOME:
			strcat(response_buffer, "    <script>\r\n");
			strcat(response_buffer, "      function send_pump_control() {\r\n");
			strcat(response_buffer, "        fetch('/pump', {\r\n");
			strcat(response_buffer, "          method: 'POST',\r\n");
			strcat(response_buffer, "          headers: { 'Content-type': 'text/plain' },\r\n");
			strcat(response_buffer, "          body: document.getElementById('relay_control').innerHTML\r\n");
			strcat(response_buffer, "        }).then(response => {\r\n");
			strcat(response_buffer, "          if (response.status != 200) window.location.reload()\r\n");
			strcat(response_buffer, "        })\r\n");
			strcat(response_buffer, "      }\r\n");
			strcat(response_buffer, "    </script>\r\n");
			strcat(response_buffer, "    <script>\r\n");
			strcat(response_buffer, "      function refresh_data() {\r\n");
			strcat(response_buffer, "        fetch('/pump/status')\r\n");
			strcat(response_buffer, "        .then(response => response.json())\r\n");
			strcat(response_buffer, "        .then(data => {\r\n");
			strcat(response_buffer, "          for (const key in data) {\r\n");
			strcat(response_buffer, "            element = document.getElementById(key)\r\n");
			strcat(response_buffer, "            element.innerHTML = data[key] == 0 ? 'OFF' : 'ON';\r\n");
			strcat(response_buffer, "            if (key == 'relay')\r\n");
			strcat(response_buffer, "              document.getElementById('relay_control').innerHTML = data[key] == 1 ? 'OFF' : 'ON'\r\n");
			strcat(response_buffer, "          }\r\n");
			strcat(response_buffer, "          if ((data['t_bottom'] == 0 && data['t_top'] == 0) || (data['t_bottom'] == 1 && data['t_top'] == 1) || data['r_bottom'] == 0)\r\n");
			strcat(response_buffer, "            document.getElementById('relay_control').setAttribute('class', 'btn shadow-sm btn-primary disabled')\r\n");
			strcat(response_buffer, "          else\r\n");
			strcat(response_buffer, "            document.getElementById('relay_control').setAttribute('class', 'btn shadow-sm btn-primary')\r\n");
			strcat(response_buffer, "        })\r\n");
			strcat(response_buffer, "      }\r\n");
			strcat(response_buffer, "      setInterval(refresh_data, 1000)\r\n");
			strcat(response_buffer, "    </script>\r\n");
			break;
		case AUTH_HOME:
			strcat(response_buffer, "    <script>\r\n");
			strcat(response_buffer, "      document.getElementById('password_input')\r\n");
			strcat(response_buffer, "      .addEventListener('keydown', async function send_password(event) {\r\n");
			strcat(response_buffer, "        if (event.keyCode == 13) {\r\n");
			strcat(response_buffer, "          fetch('/auth', {\r\n");
			strcat(response_buffer, "            method: 'POST',\r\n");
			strcat(response_buffer, "            headers: { 'Content-type': 'text/plain' },\r\n");
			strcat(response_buffer, "            body: document.getElementById('password_input').value\r\n");
			strcat(response_buffer, "          }).then(response => {\r\n");
			strcat(response_buffer, "            if (response.status == 200) window.location.reload()\r\n");
			strcat(response_buffer, "          })\r\n");
			strcat(response_buffer, "        }\r\n");
			strcat(response_buffer, "      })\r\n");
			strcat(response_buffer, "    </script>\r\n");
			break;
		case DEV_HOME:
			strcat(response_buffer, "    <script>\r\n");
			strcat(response_buffer, "      function refresh_data() {\r\n");
			strcat(response_buffer, "        fetch('/pump/dev/status')\r\n");
			strcat(response_buffer, "        .then(response => response.json())\r\n");
			strcat(response_buffer, "        .then(data => {\r\n");
			strcat(response_buffer, "          for (const key in data)\r\n");
			strcat(response_buffer, "            document.getElementById(key).innerHTML = data[key];\r\n");
			strcat(response_buffer, "        })\r\n");
			strcat(response_buffer, "      }\r\n");
			strcat(response_buffer, "      setInterval(refresh_data, 1000)\r\n");
			strcat(response_buffer, "    </script>\r\n");
			break;
		default:
			break;
	}
	switch (request_type) {
		case DEV_HOME:
		case AUTH_HOME:
		case PUMP_HOME:
			strcat(response_buffer, "    <script>\r\n");
			strcat(response_buffer, "      function toggle_theme() {\r\n");
			strcat(response_buffer, "        dataset = document.documentElement.dataset\r\n");
			strcat(response_buffer, "        dataset.bsTheme = dataset.bsTheme == 'dark' ? 'light' : 'dark'\r\n");
			strcat(response_buffer, "        localStorage.setItem('theme', dataset.bsTheme)\r\n");
			strcat(response_buffer, "      }\r\n");
			strcat(response_buffer, "    </script>\r\n");
			strcat(response_buffer, "    <script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js' integrity='sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz' crossorigin='anonymous'></script>\r\n");
			strcat(response_buffer, "  </body>\r\n");
			strcat(response_buffer, "</html>\r\n");
			break;
		default:
			break;
	}
}
