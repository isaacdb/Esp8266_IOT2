/*
Atividade4
*/

/*	Relação entre pinos da WeMos D1 R2 e GPIOs do ESP8266
	Pinos-WeMos		Função			Pino-ESP-8266
		TX			TXD				TXD/GPIO1
		RX			RXD				RXD/GPIO3
		D0			IO				GPIO16
		D1			IO, SCL			GPIO5
		D2			IO, SDA			GPIO4
		D3			IO, 10k PU		GPIO0
		D4			IO, 10k PU, LED GPIO2
		D5			IO, SCK			GPIO14
		D6			IO, MISO		GPIO12
		D7			IO, MOSI		GPIO13
		D0			IO, 10k PD, SS	GPIO15
		A0			Inp. AN 3,3Vmax	A0
*/

/* Inclusão das Bibliotecas */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"

#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include <ultrasonic.h>
#include <dht.h>

#include <sys/param.h>
#include "protocol_examples_common.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>
#include "driver/gpio.h"
#include <stdbool.h>

/* Definições e Constantes */
#define TRUE  1
#define FALSE 0
#define DEBUG TRUE 
#define LED_BUILDING	GPIO_NUM_2 
#define LED_VRD			GPIO_NUM_14 
#define BUTTON			GPIO_NUM_0
#define GPIO_OUTPUT_PIN_SEL  	((1ULL<<LED_1) | (1ULL<<LED_2))
#define GPIO_INPUT_PIN_SEL  	(1ULL<<BUTTON)

/*Parametro para conexão com o wifi, podem ser acessados atravez do comando make menuconfig*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* Bits que vao ser usados para reconhecer o estado atual da conexão wifi e de controle dos sensores*/
#define WIFI_CONNECTED_BIT       BIT0
#define WIFI_FAIL_BIT            BIT1
#define WIFI_CONNECTING_BIT      BIT2

#define SENSOR_ULTRASONIC BIT3
#define SENSOR_TERMIC     BIT4

/*Definições para o uso dos sensores*/
#define MAX_DISTANCE_CM 500
#define TRIGGER_GPIO GPIO_NUM_16
#define ECHO_GPIO GPIO_NUM_5

#define dht_gpio			GPIO_NUM_4
static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;

/*Definicoes para o socket*/
#ifdef CONFIG_EXAMPLE_IPV4
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#else
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#endif

#define PORT CONFIG_EXAMPLE_PORT

 /* Protótipos de Funções */
void app_main(void);
void task_GPIO_Blink(void* pvParameter);
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifi_init_sta(void);
void task_button(void* pvParameter);

/* Variáveis Globais */
static const char* TAG = "wifi station: ";
static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t sensor_event_group;
static int s_retry_num = 0;

/*Variaveis globais para o socket*/
static const char* TAG2 = "example";
static const char* MSG_ = " Para receber uma leitura do sensores, digite 'DIST' ou 'TEMP'. ";
static const char* payload = "Message from ESP8266 ";

QueueHandle_t buffer;//Objeto da queue

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP8266 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador (no caso de Ip dinâmico).
  ref: https://github.com/espressif/esp-idf/tree/c77c4ccf6c43ab09fd89e7c907bf5cf2a3499e3b/examples/wifi/getting_started/station
*/
static void event_handler(void* arg, esp_event_base_t event_base,
	int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		if (DEBUG)
			ESP_LOGI(TAG, "Tentando conectar ao WiFi...\r\n");
		/*
			O WiFi do ESP8266 foi configurado com sucesso.
			Agora precisamos conectar a rede WiFi local. Portanto, foi chamado a função esp_wifi_connect();
		*/
		xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
		xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
		esp_wifi_connect();
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			/*
			Se chegou aqui foi devido a falha de conexão com a rede WiFi.
			Por esse motivo, haverá uma nova tentativa de conexão WiFi pelo ESP8266.
			*/
			xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "Tentando reconectar ao WiFi...");
		}
		else {
			/*
				É necessário apagar o bit para avisar as demais Tasks que a
				conexão WiFi está offline no momento.
			*/
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
		}
		ESP_LOGI(TAG, "Falha ao conectar ao WiFi");
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		/*
			Conexão efetuada com sucesso. Busca e imprime o IP atribuido.
		*/
		ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
		ESP_LOGI(TAG, "Conectado! O IP atribuido é:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		/*
				Seta o bit indicativo para avisar as demais Tasks que o WiFi foi conectado.
		*/
		xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
	}
}

/* Inicializa o WiFi em modo cliente (Station) */
void wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate(); //Cria o grupo de eventos

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = EXAMPLE_ESP_WIFI_SSID,
			.password = EXAMPLE_ESP_WIFI_PASS
		},
	};

	/* Setting a password implies station will connect to all security modes including WEP/WPA.
		* However these modes are deprecated and not advisable to be used. Incase your Access point
		* doesn't support WPA2, these mode can be enabled by commenting below line */

	if (strlen((char*)wifi_config.sta.password)) {
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_sta finished.");
}


void task_GPIO_Blink(void* pvParameter)
{
	/*  Parâmetros de controle da GPIO da função "gpio_set_direction"
	   GPIO_MODE_OUTPUT       			//Saída
	   GPIO_MODE_INPUT        			//Entrada
	   GPIO_MODE_INPUT_OUTPUT 			//Dreno Aberto
   */

	gpio_set_direction(LED_BUILDING, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_BUILDING, 1);  //O Led desliga em nível 1;
	bool estado = 0;
	int delay_blink = 0;

	while (TRUE)
	{
		//int delay_blink = 0;
		EventBits_t event_bits = xEventGroupGetBits(s_wifi_event_group);

		// Analisa o modo do LED
		if (event_bits & WIFI_CONNECTED_BIT) {
			estado = 0;
			delay_blink = 500;
		}
		else if (event_bits & WIFI_FAIL_BIT) {
			estado = !estado;
			delay_blink = 100;
		}
		else if (event_bits & WIFI_CONNECTING_BIT) {
			estado = !estado;
			delay_blink = 500;
		}
		else {
			delay_blink = 2000;
		}

		gpio_set_level(LED_BUILDING, estado);

		vTaskDelay(delay_blink / portTICK_PERIOD_MS); //Delay de 50ms liberando scheduler;
	}
}

void task_button(void* pvParameter) {

	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BUTTON, GPIO_PULLDOWN_ONLY);

	gpio_set_direction(LED_VRD, GPIO_MODE_OUTPUT);

	while (1) {
		gpio_set_level(LED_VRD, 0);

		xEventGroupWaitBits(s_wifi_event_group, WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

		if (!gpio_get_level(BUTTON))
		{
			xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
			s_retry_num = 0;
			esp_wifi_connect();
			gpio_set_level(LED_VRD, 1);
		}

		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

void ultrasonic_test(void* pvParamters)
{
	ultrasonic_sensor_t sensor = {
		.trigger_pin = TRIGGER_GPIO,
		.echo_pin = ECHO_GPIO
	};

	ultrasonic_init(&sensor);

	while (true)
	{
		xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

		uint32_t distance;
		esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
		if (res != ESP_OK)
		{
			printf("Error: ");
			switch (res)
			{
			case ESP_ERR_ULTRASONIC_PING:
				printf("Cannot ping (device is in invalid state)\n");
				break;
			case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
				printf("Ping timeout (no device found)\n");
				break;
			case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
				printf("Echo timeout (i.e. distance too big)\n");
				break;
			default:
				printf("%d\n", res);
			}
		}
		else {
			EventBits_t event_bits = xEventGroupGetBits(sensor_event_group);

			if (event_bits & SENSOR_ULTRASONIC)
			{
				char str[10];
				sprintf(str, "%u", distance);
				xQueueSend(buffer, &str, pdMS_TO_TICKS(0));
				xEventGroupClearBits(sensor_event_group, SENSOR_ULTRASONIC);
				xEventGroupClearBits(sensor_event_group, SENSOR_TERMIC);
			}
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void dht_test(void* pvParameters)
{
	int16_t temperature = 0;
	int16_t humidity = 0;

	//gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);

	while (1)
	{
		if (dht_read_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK)
		{
			EventBits_t event_bits = xEventGroupGetBits(sensor_event_group);

			if (event_bits & SENSOR_TERMIC)
			{
				int16_t itemp = temperature / 10;
				int16_t ihum = humidity / 10;

				char hum[10];
				char temp[10];
				char msg[128] = "Humidade: ";
				char percent[10] = "%, ";
				char temperatura[20] = "Temperatura: ";
				char celsius[10] = "C.";


				sprintf(hum, "%u", ihum);
				sprintf(temp, "%u", itemp);

				strcat(msg, hum);
				strcat(msg, percent);
				strcat(msg, temperatura);
				strcat(msg, temp);
				strcat(msg, celsius);

				xQueueSend(buffer, &msg, pdMS_TO_TICKS(0));
				xEventGroupClearBits(sensor_event_group, SENSOR_ULTRASONIC);
				xEventGroupClearBits(sensor_event_group, SENSOR_TERMIC);
			}
		}			
		else
			printf("Could not read data from sensor\n");


		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

static void tcp_client_task(void* pvParameters)
{
	char rx_buffer[256];
	char rx_buffer_rcv[128];
	char addr_str[128];
	int addr_family;
	int ip_protocol;

	while (1) {

#ifdef CONFIG_EXAMPLE_IPV4
		struct sockaddr_in destAddr;
		destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;
		inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
		struct sockaddr_in6 destAddr;
		inet6_aton(HOST_IP_ADDR, &destAddr.sin6_addr);
		destAddr.sin6_family = AF_INET6;
		destAddr.sin6_port = htons(PORT);
		destAddr.sin6_scope_id = tcpip_adapter_get_netif_index(TCPIP_ADAPTER_IF_STA);
		addr_family = AF_INET6;
		ip_protocol = IPPROTO_IPV6;
		inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

		xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

		int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
		if (sock < 0) {
			ESP_LOGE(TAG2, "Unable to create socket: errno in socket %d", errno);
			break;
		}
		ESP_LOGI(TAG2, "Socket created");

		int err = connect(sock, (struct sockaddr*)&destAddr, sizeof(destAddr));
		if (err != 0) {
			ESP_LOGE(TAG2, "Socket unable to connect: errno in connect %d", errno);
			close(sock);
			continue;
		}
		ESP_LOGI(TAG2, "Successfully connected");

		bool estado = 0;
		while (1) {

			int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
			// Error occured during receiving
			if (len < 0) {
				ESP_LOGE(TAG2, "recv failed: errno %d", errno);
				break;
			}

			else {
				rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
			   // ESP_LOGI(TAG2, "Received %d bytes from %s:", len, addr_str);
			   // ESP_LOGI(TAG2, "%s", rx_buffer);
				if (len >= 4 && rx_buffer[0] == 'D' && rx_buffer[1] == 'I' && rx_buffer[2] == 'S' && rx_buffer[3] == 'T')
				{
					printf("Mensagem: ULTRASSONIC \n");
					xEventGroupSetBits(sensor_event_group, SENSOR_ULTRASONIC);
				}
				else if (len >= 4 && rx_buffer[0] == 'T' && rx_buffer[1] == 'E' && rx_buffer[2] == 'M' && rx_buffer[3] == 'P')
				{
					printf("Mensagem: TERMIC \n");
					xEventGroupSetBits(sensor_event_group, SENSOR_TERMIC);
				}
				else {
					printf("Mensagem: %s \n", MSG_);
				}
			}

			int err = send(sock, rx_buffer, strlen(rx_buffer), 0);
			if (err < 0) {
				ESP_LOGE(TAG2, "Error occured during sending: errno %d", errno);
				break;
			}

			if (xQueueReceive(buffer, &rx_buffer_rcv, pdMS_TO_TICKS(2000)) == true)//Se recebeu o valor dentro de 1seg (timeout), mostrara na tela
			{
				printf("Mensagem: %s \n", rx_buffer_rcv);
			}

			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}

		if (sock != -1) {
			ESP_LOGE(TAG2, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
		}
	}
	vTaskDelete(NULL);
}

/* Aplicação Principal (Inicia após bootloader) */
void app_main(void)
{

	sensor_event_group = xEventGroupCreate();
	char rx_buffer[256];
	/*	Sempre que utilizar o WiFi ou o Bluetooth adicione o bloco a seguir para inicializar a NVS (Non-volatile storage).
		Este espaço de memória reservado armazena dados necessários para a calibração do PHY.
		Devido ao fato de o ESP não possuir EEPROM é necessário separar um pedaço da memória de programa para armazenar
		dados não voláteis*/
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	//Configura e inicializa o WiFi.
	wifi_init_sta();

	// Cria a task responsável pelo blink LED. 
	xTaskCreate(task_GPIO_Blink, "task_GPIO_Blink", 2048, NULL, 1, NULL);

	// Cria a task responsável pelo botao. 
	xTaskCreate(task_button, "task_button", 2048, NULL, 1, NULL);

	//Cria a task responsavel pelo sensor
	xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
	xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

	//ConexaoSocket
	buffer = xQueueCreate(20, sizeof(rx_buffer)-1);//Cria a queue *buffer* com 10 slots de 4 Bytes
	xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}