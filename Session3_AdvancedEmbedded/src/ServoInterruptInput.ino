#include <Arduino.h>
#include "driver/uart.h" // this gives us direct access to ESP32's UART hardware driver

#define SERVO_PIN 32 // GPIO32 for servo signal
#define LED_PIN 2    // onboard LED (used to show interrupt activity)

#define UART_NUM UART_NUM_0 // we're using UART0 (the same one used for Serial Monitor)
#define BUF_SIZE 128         // buffer size for incoming serial data

volatile int servoPulse = 1500; // starting servo position (1.5ms = center)
volatile bool servoUpdate = false; // flag that tells loop() when to move servo

QueueHandle_t uart_queue; // queue to hold UART interrupt events (FreeRTOS thing)

// this runs in the background, triggered by UART interrupts
void uartEventTask(void *pvParameters) {
  uart_event_t event; // stores info about what kind of UART event happened
  uint8_t data[BUF_SIZE]; // buffer for received bytes

  while (1) {
    // wait here until a UART event happens (interrupt tells us)
    if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
      if (event.type == UART_DATA) { // if it's incoming data...
        
        // read however many bytes came in
        int len = uart_read_bytes(UART_NUM, data, event.size, portMAX_DELAY);
        
        // loop through each character we got
        for (int i = 0; i < len; i++) {
          char c = (char)data[i]; // convert byte to character
          
          // move servo based on input
          if (c == 'a') {
            servoPulse = 1000;  // 0 degrees
            servoUpdate = true;
          }
          else if (c == 'b') {
            servoPulse = 1500;  // 90 degrees
            servoUpdate = true;
          }
          else if (c == 'c') {
            servoPulse = 2000;  // 180 degrees
            servoUpdate = true;
          }
          
          // LED toggles every time we receive a character
          digitalWrite(LED_PIN, !digitalRead(LED_PIN)); 
        }
      }
    }
  }
}

// helper function to actually update servo PWM signal
void updateServo(int pulseWidth) {
  // so servo expects a 20ms signal period (50Hz)
  // here we convert pulse width (like 1500µs) into PWM duty cycle
  uint32_t duty = (pulseWidth * 65536) / 20000; // (pulse / period) * resolution
  ledcWrite(SERVO_PIN, duty); // update the PWM duty cycle on that pin
}

void setup() {
  pinMode(LED_PIN, OUTPUT); // LED shows when interrupts happen
  
  // setup the PWM output for the servo
  ledcAttach(SERVO_PIN, 50, 16); // 50Hz, 16-bit resolution
  updateServo(servoPulse); // center servo at startup
  
  // now we configure the UART hardware
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };
  
  // this installs the UART driver and sets up the interrupt queue
  uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0);
  uart_param_config(UART_NUM, &uart_config);
  
  // creating a background FreeRTOS task to handle UART interrupts
  xTaskCreate(uartEventTask, "uart_event_task", 2048, NULL, 12, NULL);
  
  Serial.println("UART INTERRUPT DEMO");
  Serial.println("This uses UART interrupts to handle Serial input instantly.");
  Serial.println("Send 'a', 'b', or 'c' to move the servo.");
  Serial.println("LED blinks every time a character is received.");
}

void loop() {
  // loop just checks if the interrupt task set our flag
  if (servoUpdate) {
    servoUpdate = false; // reset flag
    updateServo(servoPulse); // actually move servo

    // print confirmation so students can see it react in Serial Monitor
    Serial.print("UART Interrupt!! Pulse: ");
    Serial.print(servoPulse);
    Serial.println(" microseconds");
  }

  // note: loop() isn't constantly checking Serial, interrupts handle that!
}
