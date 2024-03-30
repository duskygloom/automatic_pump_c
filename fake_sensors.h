#ifndef FAKE_SENSORS_H
#define FAKE_SENSORS_H

/**
 * @brief
 * Fake sensors file is used to set the values of sensors manually.
 * It is a simple text file with 4 space separated integers.
 * Order of the sensor in the file:
 *      tank top, tank bottom, reservoir bottom, relay
 * 0 corresponds to OFF.
 * And any other integer corresponds to ON.
*/
#define FAKE_SENSORS_FILE   "fake_sensors.txt"


typedef enum { T_TOP, T_BOTTOM, R_BOTTOM, RELAY } sensor_t;

int get_sensor_value(sensor_t sensor);
void set_sensor_value(sensor_t sensor, int value);

void *handle_sensors(void *args);

#endif // FAKE_SENSORS_H
