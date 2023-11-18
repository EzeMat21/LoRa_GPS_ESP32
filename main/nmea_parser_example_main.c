/* NMEA Parser example, that decode data stream from GPS receiver

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nmea_parser.h"
#include "lora.h"
#include "driver/gpio.h"


//------------------------------------------------------------------------------------------------------------------------

TaskHandle_t TaskHandleLora = NULL;
TaskHandle_t TaskHandleGPS = NULL;

QueueHandle_t queueGPS_Lora;  // Cola donde el GPS le avisa al LoRa para transmitir


//------------------------------------------------------------------------------------------------------------------------
//Interrupcion por pin

#define INPUT_PIN 13
#define LED_PIN 25

int state = 0;
QueueHandle_t interputQueue;

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}


void entry_LoraTask(void *params);
void entry_GPSTask(void *params);
void entry_Boton(void *params);

//------------------------------------------------------------------------------------------------------------------------

static const char *TAG = "gps_demo";

#define TIME_ZONE (-3)   //Buenos Aires
#define YEAR_BASE (2000) //date in GPS starts from 2000

/**
 * @brief GPS Event Handler
 *
 * @param event_handler_arg handler specific arguments
 * @param event_base event base, here is fixed to ESP_NMEA_EVENT
 * @param event_id event id
 * @param event_data event specific arguments
 * 
 * 
 */

gps_t *gps = NULL;
int lat, lon;

uint8_t contador = 0;
uint8_t buffer[10];

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
 
    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        lat = gps->latitude;
        lon = gps->longitude;

        /* print information parsed from GPS statements */
        /*ESP_LOGI(TAG, "%d/%d/%d %d:%d:%d => \r\n"
                 "\t\t\t\t\t\tlatitude   = %d\r\n"
                 "\t\t\t\t\t\tlongitude = %d\r\n"
                 "\t\t\t\t\t\taltitude   = %.02fm\r\n"
                 "\t\t\t\t\t\tspeed      = %fm/s",
                 gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
                 gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
                 gps->latitude, gps->longitude, gps->altitude, gps->speed);*/
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}


void app_main(void)
{   
 
    xTaskCreate(entry_Boton, "LED_Control_Task", 2048, NULL, 1, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INPUT_PIN, gpio_interrupt_handler, (void *)INPUT_PIN);



    xTaskCreate(entry_LoraTask, "Tarea LoRa", 4096, NULL, 1, &TaskHandleLora);
    xTaskCreate(entry_GPSTask, "Tarea GPS", 4096, NULL, 1, &TaskHandleGPS);

}

void entry_GPSTask(void *params){
    

    uint8_t txBuffer = 'T';
    queueGPS_Lora = xQueueCreate(1, sizeof(txBuffer));

    if (queueGPS_Lora == 0)
    {
        printf("Fallo al crear la cola = %p\n", queueGPS_Lora);
    }

        /* NMEA parser configuration */
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    /* init NMEA parser library */

    while(1){

        nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
        /* register event handler for NMEA parser library */
        nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

        vTaskDelay(1000/ portTICK_PERIOD_MS);


        /* unregister event handler */
        nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
        /* deinit NMEA parser library */
        nmea_parser_deinit(nmea_hdl);

        buffer[0] = contador;
        contador++;

        //Bytes del 1 al 4 corresponden a la latitud

        for(uint8_t i=4; i>0;i--){
            buffer[i] = lat;
            lat = lat >>8;
        }

        //Bytes del 5 al 8 corresponden a la longitud

        for(uint8_t i=8; i>4;i--){
        buffer[i] = lon;
        lon = lon >>8;
        }

        xQueueSend(queueGPS_Lora, &txBuffer, (TickType_t)10);

        vTaskDelay(20000/ portTICK_PERIOD_MS);
        printf("Hola estoy GPS\n");

    }
    
}


void entry_LoraTask(void *params){

    //Configuracion del LoRa
   lora_init();
   lora_set_frequency(915e6);
   lora_set_bandwidth(125E3);          //Ancho de banda de 7.8kHz
   lora_set_spreading_factor(7);      //SF = 12
   lora_set_coding_rate(5);            //coding rate 4/5
   lora_set_preamble_length(8);
   lora_enable_crc();
    
    uint8_t rxBuffer;

    while(1){
        if(xQueueReceive(queueGPS_Lora, &rxBuffer, (TickType_t)10)){
            lora_send_packet((uint8_t*)buffer, 10);
            printf("packet sent...\n");

            for(int i=0;i<10;i++){
                printf("%u\n",buffer[i]);
            }
        }
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}


void entry_Boton(void *params)
{   
    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(INPUT_PIN);
    gpio_set_direction(INPUT_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(INPUT_PIN);
    gpio_pullup_dis(INPUT_PIN);
    gpio_set_intr_type(INPUT_PIN, GPIO_INTR_POSEDGE);

    interputQueue = xQueueCreate(10, sizeof(int));
    uint8_t pinNumber, count = 0;
    while (true)
    {
        if (xQueueReceive(interputQueue, &pinNumber, 100))
        {
            printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, count++, gpio_get_level(INPUT_PIN));
            buffer[9] = count;
            gpio_set_level(LED_PIN, gpio_get_level(INPUT_PIN));
        }

        //printf("Hola\n");
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}