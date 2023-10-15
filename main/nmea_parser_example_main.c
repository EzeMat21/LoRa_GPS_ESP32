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


/*void task_tx(void *p)
{
      int registro = lora_read_reg(0x1d);
      printf("registro: %d\n", registro);
      uint8_t contador = 0;
   for(;;) {
      contador++;
      vTaskDelay(pdMS_TO_TICKS(5000));
      
      snprintf(buffer, 13, "Paquete n%d", contador);
      lora_send_packet((uint8_t*)buffer, 13);
      printf("packet sent...\n");
      printf("%s\n",buffer);
   }
}*/

void app_main(void)
{

    //Configuracion del LoRa
   lora_init();
   lora_set_frequency(915e6);
   lora_set_bandwidth(125e3);          //Ancho de banda de 7.8kHz
   lora_set_spreading_factor(12);      //SF = 12
   lora_set_coding_rate(5);            //coding rate 4/5
   lora_set_preamble_length(8);
   lora_enable_crc();
   //xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, NULL);


    /* NMEA parser configuration */
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    /* init NMEA parser library */


    uint8_t contador = 0;
    uint8_t buffer[10];

    while(1){

    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    /* register event handler for NMEA parser library */
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

    vTaskDelay(1400/ portTICK_PERIOD_MS);


    /* unregister event handler */
    nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
    /* deinit NMEA parser library */
    nmea_parser_deinit(nmea_hdl);

    printf("%d\n", lat);
    printf("%d\n", lon);

      //snprintf(buffer, 10, "P%d%d%d", contador,lat,lon);
    
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

      lora_send_packet((uint8_t*)buffer, 10);
      printf("packet sent...\n");

      for(int i=0;i<10;i++){
        printf("%u\n",buffer[i]);
      }

    vTaskDelay(10000/ portTICK_PERIOD_MS);
    }


    
}

