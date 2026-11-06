// Домашка 6.1: ті самі три LED, що в 2.3 (200 / 500 / 1000 мс), але кожен — окрема задача FreeRTOS.
// Перевірка за логом, як у 2.3: за 10 с має бути 50 / 20 / 10 перемикань.
#include <Arduino.h>  // разом з ним приходять freertos/FreeRTOS.h і freertos/task.h

struct Led {
    uint8_t pin;                // пін GPIO
    uint32_t ms;                // період перемикання
    volatile uint32_t toggles;  // лічильник перемикань; пише лише задача цього LED
};

// Глобальний масив: живе весь час роботи, тому адреси елементів можна віддати задачам
Led leds[] = {{18, 200, 0}, {17, 500, 0}, {16, 1000, 0}};
constexpr uint32_t REPORT_MS = 10000;

// Одна функція на три задачі; яку саме Led обслуговувати, приходить через arg
void blinkTask(void *arg) {
    auto *led = static_cast<Led *>(arg);   // void* → Led*: «за цією адресою лежить Led»
    bool on = false;                       // у кожної задачі свій стек, тож свій on
    TickType_t last = xTaskGetTickCount(); // «о котрій стартували», у тіках (1 тік = 1 мс)
    for (;;) {
        on = !on;
        digitalWrite(led->pin, on ? HIGH : LOW);  // led — адреса, тому стрілка
        led->toggles++;
        vTaskDelayUntil(&last, pdMS_TO_TICKS(led->ms));  // старт кожні ms, робота всередині періоду
    }
}

// Звіт раз на 10 с: скільки перемикань зробив кожен LED. Друкує лише ця задача, тож м'ютекс не потрібен
void reportTask(void *) {
    uint32_t prev[3] = {0, 0, 0};
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(REPORT_MS));
        Serial.printf("[%lu мс] перемикань за 10 с:", (unsigned long)millis());
        for (int i = 0; i < 3; i++) {
            uint32_t now = leds[i].toggles;  // лише читаємо: лічильник змінює задача LED
            Serial.printf(" GPIO%u=%lu", leds[i].pin, (unsigned long)(now - prev[i]));
            prev[i] = now;
        }
        Serial.println();
    }
}

void setup() {
    Serial.begin(115200);
    for (auto &led : leds) {  // & — сам елемент масиву, не копія: його адреса живе весь час
        pinMode(led.pin, OUTPUT);
        // функція, ім'я, стек 2048 Б, параметр, пріоритет 1, хендл не потрібен, ядро 1
        xTaskCreatePinnedToCore(blinkTask, "blink", 2048, &led, 1, nullptr, 1);
    }
    xTaskCreatePinnedToCore(reportTask, "report", 4096, nullptr, 1, nullptr, 1);  // printf їсть стек
}

void loop() {
    vTaskDelete(nullptr);  // задача loop більше не потрібна: усе роблять задачі вище
}
