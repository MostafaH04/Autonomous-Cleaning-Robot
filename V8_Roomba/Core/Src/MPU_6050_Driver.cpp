#include "MPU_6050_Driver.hpp"

MPU_6050::MPU_6050(bool setAddressHigh, I2C_HandleTypeDef *hi2c_in,
  GPIO_TypeDef* addressPinReg, uint16_t addressPin):
  addressPin(addressPinReg), addressPinVal(addressPin)
{
  if (setAddressHigh)
  {
    address = ADDRESS_HIGH;
  }
  else
    address= ADDRESS_LOW;

  imuData = new IMU_Data();

  hi2c = hi2c_in;
} 

MPU_6050::~MPU_6050()
{
	delete imuData;
}

void MPU_6050::calibrateMPU(void)
{
  // take 4000 measurements and average them out;
  float buffAx = 0, buffAy = 0, buffAz = 0;
  float buffGx = 0, buffGy = 0, buffGz = 0;
  const int TOTAL_MEASUREMENTS = 200;

  for (int measIter = 0; measIter < TOTAL_MEASUREMENTS; measIter ++)
  {
    readTempRegisters();
	  readGyroRegisters();
	  readAccelRegisters();

    buffAx += imuData->accX;
    buffAy += imuData->accY;
    buffAz += imuData->accZ - LSB_g * (address == ADDRESS_HIGH?-1:1);

    buffGx += imuData->gyroX;
    buffGy += imuData->gyroY;
    buffGz += imuData->gyroZ;
  }

  calibAx = buffAx / TOTAL_MEASUREMENTS;
  calibAy = buffAy / TOTAL_MEASUREMENTS;
  calibAz = buffAz / TOTAL_MEASUREMENTS;

  calibGx = buffGx / TOTAL_MEASUREMENTS;
  calibGy = buffGy / TOTAL_MEASUREMENTS;
  calibGz = buffGz / TOTAL_MEASUREMENTS;
}

void MPU_6050::correctData(void)
{
  imuData->accX = (imuData->accX - calibAx)/LSB_g;
  imuData->accY = (imuData->accY - calibAy)/LSB_g;
  imuData->accZ = (imuData->accZ - calibAz)/LSB_g;

  imuData->gyroX = (imuData->gyroX - calibGx)/LSB_deg_s * 0.01745329; //deg to rad
  imuData->gyroY = (imuData->gyroY - calibGy)/LSB_deg_s * 0.01745329;
  imuData->gyroZ = (imuData->gyroZ - calibGz)/LSB_deg_s * 0.01745329;
}

void MPU_6050::startPins()
{
  if (address == ADDRESS_HIGH)
  {
	HAL_GPIO_WritePin(addressPin, addressPinVal, GPIO_PIN_SET);
  }
  else
  {
	  HAL_GPIO_WritePin(addressPin, addressPinVal, GPIO_PIN_RESET);
  }
}

bool MPU_6050::initialize(void)
{
  sleepMode(false);
  resetRegisters();
  disableTemp(false);
  setFullScaleAccel(ACCEL_FULLSCALE_16);
  setFullScaleGyro(GYRO_FULLSCALE_1000);
  selectClockSource(INTERNAL_8MHZ);
  calibrateMPU();
//  if (!selfTest())
//  {
//    return INIT_FAIL;
//  }
  return INIT_SUCCESS;
}

double factoryTrimGyro(uint8_t testVal)
{
  return 25 * 131 * pow(1.046, testVal-1);
}

double factoryTrimAccel(uint8_t testVal)
{
  return 4096 * 0.34 * pow((0.92/0.34), (testVal-1)/(pow(2,5)-2));
}

bool MPU_6050::selfTest(void)
{
  uint8_t test_recv[4];

  float GyroXDisabled, GyroYDisabled, GyroZDisabled;
  float AccelXDisabled, AccelYDisabled, AccelZDisabled;
  
  uint8_t normalMeas[1] = {0b00011000};
  HAL_I2C_Mem_Write(hi2c, (address<<1), GYRO_CONFIG, 1,
    normalMeas, 1, HAL_MAX_DELAY);
  //osDelay(50);

  readAccelRegisters();
  readGyroRegisters();

  GyroXDisabled = imuData->gyroX;
  GyroYDisabled = imuData->gyroY;
  GyroZDisabled = imuData->gyroZ;

  AccelXDisabled = imuData->accX;
  AccelYDisabled = imuData->accY;
  AccelZDisabled = imuData->accZ;

  float StrGyroX, StrGyroY, StrGyroZ;
  float StrAccelX, StrAccelY, StrAccelZ;

  uint8_t selfTest = 0b11111000;
  HAL_I2C_Mem_Write(hi2c, (address<<1), GYRO_CONFIG, 1,
    &selfTest, sizeof(selfTest), HAL_MAX_DELAY);

  //osDelay(50);

  readAccelRegisters();
  readGyroRegisters();


  StrGyroX = imuData->gyroX - GyroXDisabled;
  StrGyroY = imuData->gyroY - GyroYDisabled;
  StrGyroZ = imuData->gyroZ - GyroZDisabled;

  StrAccelX = imuData->accX - AccelXDisabled;
  StrAccelY = imuData->accY - AccelYDisabled;
  StrAccelZ = imuData->accZ - AccelZDisabled;

  HAL_I2C_Mem_Write(hi2c, (address<<1), GYRO_CONFIG, 1,
    normalMeas, sizeof(normalMeas), HAL_MAX_DELAY);

  HAL_I2C_Mem_Read(hi2c, (address<<1) | 0x01, SELF_TEST_X, 1,
    test_recv, 4, HAL_MAX_DELAY);
  
  uint8_t test_a_x, test_a_y, test_a_z;
  uint8_t test_g_x, test_g_y, test_g_z;

  test_a_x = (test_recv[0] & 0b11100000) >> 1 
    | ((test_recv[3] & 0b00110000) << 4);
  test_a_y = (test_recv[1] & 0b11100000) >> 1 
    | ((test_recv[3] & 0b00001100) << 2);
  test_a_z = (test_recv[2] & 0b11100000) >> 1
    | ((test_recv[3] & 0b00000011));
  
  test_g_x = (test_recv[0] & 0b00011111);
  test_g_y = (test_recv[1] & 0b00011111);
  test_g_z = (test_recv[2] & 0b00011111);

  float factTrimGx = 0, factTrimGy = 0, factTrimGz = 0;
  float factTrimAx = 0, factTrimAy = 0, factTrimAz = 0;

  if (test_g_x != 0)
    factTrimGx = factoryTrimGyro(test_g_x);
  if (test_g_y != 0)
    factTrimGy = factoryTrimGyro(test_g_y);
  if (test_g_z != 0)
    factTrimGz = factoryTrimGyro(test_g_z);

  if (test_a_x != 0)
    factTrimAx = factoryTrimAccel(test_a_x);
  if (test_a_y != 0)
    factTrimAy = factoryTrimAccel(test_a_y);
  if (test_a_z != 0)
    factTrimAz = factoryTrimAccel(test_a_z);

  float accXRes = (StrAccelX - factTrimAx)/factTrimAx * 100;
  float accYRes = (StrAccelY - factTrimAy)/factTrimAy * 100;
  float accZRes = (StrAccelZ - factTrimAz)/factTrimAz * 100;

  float gyroXRes = (StrGyroX - factTrimGx)/factTrimGx * 100;
  float gyroYRes = (StrGyroY - factTrimGy)/factTrimGy * 100;
  float gyroZRes = (StrGyroZ - factTrimGz)/factTrimGz * 100;

  bool passAccX = abs(accXRes) < 14;
  bool passAccY = abs(accYRes) < 14;
  bool passAccZ = abs(accZRes) < 14;

  bool passGyroX = abs(gyroXRes) < 14;
  bool passGyroY = abs(gyroYRes) < 14;
  bool passGyroZ = abs(gyroZRes) < 14;

  return passAccX && passAccY && passAccZ && passGyroX
    && passGyroY && passGyroZ;
}

void MPU_6050::setSampleRate(uint16_t sample_rate_hz)
{
  uint8_t rate_divider[1] = {(uint8_t)((GYRO_OUTPUT_RATE_HZ / sample_rate_hz) - 1)};
  HAL_I2C_Mem_Write(hi2c, (address<<1), SMPLRT_DIV, 1,
    rate_divider, 1, HAL_MAX_DELAY);
}

void MPU_6050::setFullScaleGyro(Gyro_FullScale gyro_fs_setting)
{
  uint8_t config[1] = {(uint8_t)(gyro_fs_setting << 3)};
  HAL_I2C_Mem_Write(hi2c, (address<<1), GYRO_CONFIG, 1,
    config, 1, HAL_MAX_DELAY);

  switch(gyro_fs_setting)
  {
    case 0x00: // 250
      LSB_deg_s = GYRO_FS_250_RANGE;
      break; 
    case 0x01: //500
      LSB_deg_s = GYRO_FS_500_RANGE;
      break;
    case 0x02: // 1000
      LSB_deg_s = GYRO_FS_1000_RANGE;
      break;
    case 0x03: // 2000
      LSB_deg_s = GYRO_FS_2000_RANGE;
      break;
  }
}

void MPU_6050::setFullScaleAccel(Accel_FullScale accel_fs_setting)
{
  uint8_t config[1] = {(uint8_t)(accel_fs_setting << 3)};
  HAL_I2C_Mem_Write(hi2c, (address<<1), ACCEL_CONFIG, 1,
    config, 1, HAL_MAX_DELAY);

  switch(accel_fs_setting)
  {
    case 0x00: // 2
      LSB_g = ACCEL_FS_2_RANGE;
      break; 
    case 0x01: // 4
      LSB_g = ACCEL_FS_4_RANGE;
      break;
    case 0x02: // 8
      LSB_g = ACCEL_FS_8_RANGE;
      break;
    case 0x03: // 16
      LSB_g = ACCEL_FS_16_RANGE;
      break;
  }
}


void MPU_6050::readAccelRegisters(void)
{
  int16_t accelX, accelY, accelZ;
  uint8_t accelRec[6];

  HAL_I2C_Mem_Read(hi2c,(address<<1) | 0x01, ACCEL_XOUT_H, 1,
    accelRec, 6, HAL_MAX_DELAY);
  
  accelX = (int16_t)(accelRec[0] << 8 | accelRec[1]);
  accelY = (int16_t)(accelRec[2] << 8 | accelRec[3]);
  accelZ = (int16_t)(accelRec[4] << 8 | accelRec[5]);

  imuData->accX = accelX;
  imuData->accY = accelY;
  imuData->accZ = accelZ;
}

void MPU_6050::readGyroRegisters(void)
{
  int16_t gyroX, gyroY, gyroZ;
  uint8_t gyroRec[6];

  HAL_I2C_Mem_Read(hi2c,(address<<1) | 0x01,GYRO_XOUT_H, 1,
    gyroRec, 6, HAL_MAX_DELAY);
  
  gyroX = (int16_t)(gyroRec[0] << 8 | gyroRec[1]);
  gyroY = (int16_t)(gyroRec[2] << 8 | gyroRec[3]);
  gyroZ = (int16_t)(gyroRec[4] << 8 | gyroRec[5]);

  imuData->gyroX = gyroX;
  imuData->gyroY = gyroY;
  imuData->gyroZ = gyroZ;
}

void MPU_6050::readTempRegisters(void)
{
  int16_t temp;
  uint8_t tempRecv[2];

  HAL_I2C_Mem_Read(hi2c, (address<<1) | 0x01, TEMP_OUT_H, 1,
    tempRecv, 2, HAL_MAX_DELAY);

  temp = (int16_t)(tempRecv[0] << 8 | tempRecv[1]);
  temp = temp / 340 + 36.53;

  imuData->temp = temp;
}

void MPU_6050::updateData(void)
{
  readTempRegisters();
  readGyroRegisters();
  readAccelRegisters();
  correctData();
}

void MPU_6050::resetRegisters(void)
{
  uint8_t resetRegisters[1] = {0b00000111};
  HAL_I2C_Mem_Write(hi2c, (address<<1), SIGNAL_PATH_RESET, 1,
    resetRegisters, 1, HAL_MAX_DELAY);
}

void MPU_6050::sleepMode(bool sleep)
{
  uint8_t pwr_mgmt[1] = {0x00};
  HAL_I2C_Mem_Read(hi2c, (address<<1) | 0x01, PWR_MGMT_1, 1,
    pwr_mgmt, 1, HAL_MAX_DELAY);
  
  pwr_mgmt[0] = (pwr_mgmt[0] & 0b10111111) | ((sleep?0x01:0x00) << 6);

  HAL_I2C_Mem_Write(hi2c, (address<<1), PWR_MGMT_1, 1,
    pwr_mgmt, 1, HAL_MAX_DELAY);
}

void MPU_6050::selectClockSource(Clock_Select clock_sel)
{
  uint8_t pwr_mgmt[1] = {0x00};
  HAL_I2C_Mem_Read(hi2c, (address<<1) | 0x01, PWR_MGMT_1, 1,
    pwr_mgmt, 1, HAL_MAX_DELAY);
  
  pwr_mgmt[0] = (pwr_mgmt[0]&0b11111000) | clock_sel;

  HAL_I2C_Mem_Write(hi2c, (address<<1), PWR_MGMT_1, 1,
    pwr_mgmt, 1, HAL_MAX_DELAY);
}

void MPU_6050::disableTemp(bool disable)
{
  uint8_t pwr_mgmt[1] = {0x00};
  HAL_I2C_Mem_Read(hi2c, (address<<1) | 0x01, PWR_MGMT_1, 1,
    pwr_mgmt, 1, HAL_MAX_DELAY);
  
  pwr_mgmt[0] = (pwr_mgmt[0] & 0b11110111) | (disable?0x01:0x00) << 3;

  HAL_I2C_Mem_Write(hi2c, (address<<1), PWR_MGMT_1, 1,
    pwr_mgmt, 1, HAL_MAX_DELAY);
}

void MPU_6050::sleepCycle(Wake_Up_Frequency cycleFreq)
{
  uint8_t pwr_mgmt2[1] = {(uint8_t)(cycleFreq << 6)};
  HAL_I2C_Mem_Write(hi2c, (address<<1), PWR_MGMT_2, 1,
    pwr_mgmt2, 1, HAL_MAX_DELAY);
}

IMU_Data* MPU_6050::generateNewResult(void)
{
  updateData();
  return imuData;
}