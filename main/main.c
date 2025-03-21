/*
* LED blink with FreeRTOS
*/
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

const int trigger = 13;
const int echo = 12;

QueueHandle_t xQueueTime;
SemaphoreHandle_t xSemaphoreTrigger;
QueueHandle_t xQueueDistance;

void pin_callback(uint gpio, uint32_t events){
int tempo;
if (events & GPIO_IRQ_EDGE_RISE){
tempo = to_us_since_boot(get_absolute_time());
xQueueSendFromISR(xQueueTime, &tempo, 0);
}if (events & GPIO_IRQ_EDGE_FALL) {
tempo = to_us_since_boot(get_absolute_time());
xQueueSendFromISR(xQueueTime, &tempo, 0);
}
}

void echo_task(void *p){
gpio_init(echo);
gpio_set_dir(echo, GPIO_IN);
int tempo_inicial;
int tempo_final;
int pos;
int diferenca;

while (true){
if (xQueueReceive(xQueueTime, &tempo_inicial, pdMS_TO_TICKS(80))) {
if(xQueueReceive(xQueueTime, &tempo_final, pdMS_TO_TICKS(50))){
diferenca = tempo_final - tempo_inicial;
pos = diferenca * 0.017015;
xQueueSend(xQueueDistance, &pos, 0);
}
}
}
}

void trigger_task(void *p){
gpio_init(trigger);
gpio_set_dir(trigger, GPIO_OUT);

while (true) {
gpio_put(trigger, 1);
vTaskDelay(pdMS_TO_TICKS(10));
gpio_put(trigger, 0);
vTaskDelay(pdMS_TO_TICKS(10));
xSemaphoreGive(xSemaphoreTrigger);

}
}


void oled1_btn_led_init(void) {
gpio_init(LED_1_OLED);
gpio_set_dir(LED_1_OLED, GPIO_OUT);

gpio_init(LED_2_OLED);
gpio_set_dir(LED_2_OLED, GPIO_OUT);

gpio_init(LED_3_OLED);
gpio_set_dir(LED_3_OLED, GPIO_OUT);

gpio_init(BTN_1_OLED);
gpio_set_dir(BTN_1_OLED, GPIO_IN);
gpio_pull_up(BTN_1_OLED);

gpio_init(BTN_2_OLED);
gpio_set_dir(BTN_2_OLED, GPIO_IN);
gpio_pull_up(BTN_2_OLED);

gpio_init(BTN_3_OLED);
gpio_set_dir(BTN_3_OLED, GPIO_IN);
gpio_pull_up(BTN_3_OLED);
}


void oled_task(void *p){
ssd1306_init();
ssd1306_t disp;
gfx_init(&disp, 128, 32);
//oled1_btn_led_init();
int pos;
char pos_str[20];
while (true){
if (xSemaphoreTake(xSemaphoreTrigger, pdMS_TO_TICKS(50)) == pdTRUE){
xQueueReceive(xQueueDistance, &pos, 0);
if(pos > 400){
    gfx_clear_buffer(&disp);
    gfx_draw_string(&disp, 0, 0, 1, "erro");
    gfx_show(&disp); 
}else{  
    snprintf(pos_str, sizeof(pos_str), "%d", pos);
    gfx_clear_buffer(&disp);
    gfx_draw_string(&disp, 0, 0, 1, pos_str);
    gfx_draw_line(&disp, 10, 20, pos, 20);
    gfx_show(&disp); 
}
}
}
}


int main() {
stdio_init_all();

xQueueTime = xQueueCreate(32, sizeof(int));
xQueueDistance = xQueueCreate(32, sizeof(int));
xSemaphoreTrigger = xSemaphoreCreateBinary();

gpio_set_irq_enabled_with_callback(echo, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &pin_callback);
xTaskCreate(echo_task , "echo", 4095, NULL, 1, NULL);
xTaskCreate(trigger_task , "trigger", 4095, NULL, 1, NULL);
xTaskCreate(oled_task, "oled", 4095, NULL, 1, NULL);

vTaskStartScheduler();

while (true)
;
}

