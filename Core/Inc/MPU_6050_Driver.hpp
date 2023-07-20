#ifndef MPU_6050_DRIVER
#define MPU_6050_DRIVER

#include "main.h"
#include "i2c.h"
#include <stdint.h>  

#define PWR_MGMT_1 0x6B
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C

#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H 0x41
#define TEMP_OUT_L 0x42
#define GYRO_XOUT_H 0x43
#define GYRO_XOUT_L 0x44
#define GYRO_YOUT_H 0x45
#define GYRO_YOUT_L 0x46
#define GYRO_ZOUT_H 0x47
#define GYRO_ZOUT_L 0x48

const uint16_t ADDRESS_LOW = 0x68;
const uint16_t ADDRESS_HIGH = 0x69;

struct IMU_Data{
    IMU_Data();
    float AccX, AccY, AccZ;
    float GyroX, GyroY, GyroZ;
};

class MPU_6050
{
    private:
        uint16_t address;
        uint16_t addressSet;

        float AccErrorX, AccErrorY;
        float GyroErrorX, GyroErrorY, GyroErrorZ;
        float LSB_g;
        float LSB_deg_s;

        IMU_Data* imuData;

        I2C_HandleTypeDef hi2c;
        
        void fullScaleOperation(bool fullScale);

    public:
        MPU_6050(uint16_t addressSet_in, bool setAddressHigh, I2C_HandleTypeDef hi2c);

        void MPU_6050_begin();

        IMU_Data* generateNewResult();

};

#endif