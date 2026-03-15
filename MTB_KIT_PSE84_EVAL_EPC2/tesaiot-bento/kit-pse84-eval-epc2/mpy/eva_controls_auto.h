/*******************************************************************************
 * File Name: eva_controls_auto.h
 *
 * Description: Lightweight auto-push task for Eva Kit Controls Tab.
 *              Reads Potentiometer (ADC), GPIO Buttons, and LED state
 *              on CM33_NS and pushes to CM55 via IPC every 100ms.
 *
 *              Does NOT touch I2C/I3C - safe for Eva Kit where CM55 owns SCB0.
 *              CapSense + BMI270 are read by CM55 cm55_sensor_poll instead.
 *
 *******************************************************************************/

#ifndef EVA_CONTROLS_AUTO_H
#define EVA_CONTROLS_AUTO_H

/**
 * Create and start the Eva Controls auto-push task.
 * Call once from main(), before vTaskStartScheduler().
 */
void eva_controls_auto_create(void);

#endif /* EVA_CONTROLS_AUTO_H */
