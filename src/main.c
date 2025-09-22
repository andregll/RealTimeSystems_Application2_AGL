/* --------------------------------------------------------------
   Application: 02 - Rev2
   Release Type: Baseline Preemption
   Class: Real Time Systems - Su 2025
   Author: [M Borowczak] 
   Email: [mike.borowczak@ucf.edu]
   Company: [University of Central Florida]
   Website: theDRACOlab.com
   AI Use: Commented inline -- None
---------------------------------------------------------------*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"


// TODO1: ADD IN additional INCLUDES BELOW
#include "driver/adc.h"
#include "math.h"
// TODO1: ADD IN additional INCLUDES ABOVE


// TODO2: ADD IN LDR_PIN to gpio pin 32
#define LDR_PIN GPIO_NUM_32


#define LED_PIN GPIO_NUM_2  // Using GPIO2 for the LED

// TODO3: ADD IN LDR_ADC_CHANNEL -- if you used gpio pin 32 it should map to ADC1_CHANNEL4
#define LDR_ADC_CHANNEL ADC1_CHANNEL_4




// TODO99: Consider Adding AVG_WINDOW and SENSOR_THRESHOLD as global defines
// TODO99: Consider Adding AVG_WINDOW and SENSOR_THRESHOLD as global defines
#define AVG_WINDOW 10
#define SENSOR_THRESHOLD 200 // Threshold for detecting a light intrusion event (in lux)











//TODO9: Adjust Task to blink an LED at 1 Hz (1000 ms period: 500 ms ON, 500 ms OFF);
//Consider supressing the output
void led_task(void *pvParameters) {
    while (1) {
        // Turn the LED on
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));

        // Turn the LED off
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelete(NULL);
}





















// Task to print a status message every 1000 ms (1 second)
// Represents a periodic system log entry.
void print_status_task(void *pvParameters) {
    int counter = 0;
    while (1) {
        printf("System Status: NORMAL. Heartbeat count: %d\n", counter++);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 ms
    }
    vTaskDelete(NULL);
}














// High-priority task for sensor reading every 200ms
// Represents the intrusion detection system.
void sensor_task(void *pvParameters) {
    // Variables to compute LUX
    int raw;
    float Vmeasure = 0.0;
    float Rmeasure = 0.0;
    float lux = 0.0;
    
    // Constants for LUX calculation
    const float V_SOURCE = 3.3;
    const float R_FIXED = 10000.0; // 10kOhm fixed resistor
    const float GAMMA = 0.7;
    const float RL = 50.0;

    // Variables for moving average
    float lux_readings[AVG_WINDOW] = {0}; // CORRECTED: Consistent name
    int idx = 0;
    float sum = 0;
    
    // Pre-fill the readings array with an initial sample to avoid startup anomaly
    for(int i = 0; i < AVG_WINDOW; ++i) {
        raw = adc1_get_raw(LDR_ADC_CHANNEL);
        Vmeasure = (raw / 4095.0) * V_SOURCE;
        Rmeasure = (Vmeasure * R_FIXED) / (V_SOURCE - Vmeasure);
        lux = pow(10, (log10(Rmeasure / RL) / GAMMA));
        
        lux_readings[i] = lux; // CORRECTED: Consistent name
        sum += lux_readings[i];
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay between initial readings
    }

    const TickType_t periodTicks = pdMS_TO_TICKS(200); // 200 ms period
    TickType_t lastWakeTime = xTaskGetTickCount(); // Initialize last wake time

    while (1) {
        // Read current sensor value
        raw = adc1_get_raw(LDR_ADC_CHANNEL);

        // Compute LUX
        Vmeasure = (raw / 4095.0) * V_SOURCE;
        if (V_SOURCE - Vmeasure <= 0) {
             Rmeasure = R_FIXED * 1000;
        } else {
             Rmeasure = (Vmeasure * R_FIXED) / (V_SOURCE - Vmeasure);
        }
        lux = pow(10, (log10(Rmeasure / RL) / GAMMA));
       
        // Update moving average buffer 
        sum -= lux_readings[idx];         // CORRECTED: Consistent name
        lux_readings[idx] = lux;          // CORRECTED: Consistent name
        sum += lux;
        idx = (idx + 1) % AVG_WINDOW;
        float avg_lux = sum / AVG_WINDOW; // CORRECTED: Consistent name
        
        // Check threshold and print alert if exceeded
        if (avg_lux > SENSOR_THRESHOLD) {
            printf("\n**ALERT: Possible Enclosure Breach! Light level high: %.2f lux**\n\n", avg_lux);
        } else {
            // Optional: Print average for debugging - THIS NOW WORKS
            // printf("Avg Lux: %.2f\n", avg_lux); // CORRECTED: avg_lux is now declared
        }
        
        vTaskDelayUntil(&lastWakeTime, periodTicks);
    }
    vTaskDelete(NULL);
}

void app_main() {
    // Initialize LED GPIO     
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    // TODO4 : Initialize LDR PIN as INPUT [2 lines mirroring those above]
    gpio_reset_pin(LDR_PIN);
    gpio_set_direction(LDR_PIN, GPIO_MODE_INPUT);

    // TODO5 : Set ADC1's resolution by calling:
    // function adc1_config_width(...) 
    // with parameter ADC_WIDTH_BIT_12
    adc1_config_width(ADC_WIDTH_BIT_12);



    // TODO6: Set the the input channel to 11 DB Attenuation using
    // function adc1_config_channel_atten(...,...) 
    // with parameters LDR_ADC_CHANNEL and ADC_ATTEN_DB_11
    adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN_DB_11);



    // Instantiate/ Create tasks: 
    // . pointer to task function, 
    // . descriptive name, [has a max length; located in the FREERTOS_CONFIG.H]
    // . stack depth, 
    // . parameters [optional] = NULL 
    // . priority [0 = low], 
    // . pointer referencing this created task [optional] = NULL
    // Learn more here https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/01-xTaskCreate
    
    // TODO7: Pin tasks to core 1    
    // Convert these xTaskCreate function calls to  xTaskCreatePinnedToCore() function calls
    // The new function takes one more parameter at the end (0 or 1);
    // pin all your tasks to core 1 

    // This is a special (custom) espressif FreeRTOS function
    // . pointer to task function, 
    // . descriptive name, [has a max length; located in the FREERTOS_CONFIG.H]
    // . stack depth, 
    // . parameters [optional] = NULL 
    // . priority [0 = low], 
    // . pointer referencing this created task [optional] = NULL
    // . core [0,1] to pin task too 
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_additions.html#_CPPv423xTaskCreatePinnedToCore14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC12TaskHandle_tK10BaseType_t
    xTaskCreatePinnedToCore(print_status_task, "STATUS_LOG", 2048, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(led_task, "LED_HEARTBEAT", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(sensor_task, "INTRUSION_DET", 4096, NULL, 2, NULL, 1);

    // TODO8: Make sure everything still works as expected before moving on to TODO9 (above).

    //TODO12 Add in new Sensor task; make sure it has the correct priority to preempt 
    //the other two tasks.


    //TODO13: Make sure the output is working as expected and move on to the engineering
    //and analysis part of the application. You may need to make modifications for experiments. 
    //Make sure you can return back to the working version!
}